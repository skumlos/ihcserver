#include "IHCHTTPServerWorker.h"
#include "utils/TCPSocket.h"
#include "HTTPRequest.h"
#include "Configuration.h"
#include "IHCServer.h"
#include "IHCServerDefs.h"
#include "IHCEvent.h"
#include "IHCIO.h"
#include "IHCHTTPServer.h"
#include "3rdparty/base64/base64.h"
#include "utils/ms.h"
#include <errno.h>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <openssl/sha.h>
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"
pthread_mutex_t IHCHTTPServerWorker::m_tokenMapMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t IHCHTTPServerWorker::m_allMutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int IHCHTTPServerWorker::m_all = 0;
std::map<std::string,Userlevel::UserlevelToken*> IHCHTTPServerWorker::m_tokens;

IHCHTTPServerWorker::IHCHTTPServerWorker(TCPSocket* connectedSocket) :
	m_socket(connectedSocket),
	m_ihcServer(IHCServer::getInstance()),
	m_eventMutex(NULL),
	m_eventCond(NULL)
{
	start();
}

IHCHTTPServerWorker::~IHCHTTPServerWorker() {
	delete m_socket;
	while(!m_events.empty()) {
		delete m_events.back();
		m_events.pop_back();
	}
}

void IHCHTTPServerWorker::thread() {
	std::string webroot = Configuration::getInstance()->getWebroot();
	try {
		int slept_s = 0;
		while(m_socket->poll(1000)) {
			if(slept_s >= 10) {
				throw false;
			}
			sleep(1);
			++slept_s;
		};
		std::string buffer = "";
		while(m_socket->peek() > 0) {
			std::string buf = "";
			m_socket->recv(buf);
			buffer += buf;
			usleep(100*1000);
		}
		if(buffer != "") {
			HTTPRequest req(buffer);
//			printf("Buffer:\n%s\n",buffer.c_str());
//			printf("Request type %d, URI: %s. Payload: %s.\n",req.getRequestType(),req.getRequestURI().c_str(),req.getPayload().c_str());
			std::string requestURI = req.getRequestURI();
			if(requestURI == "/ihcrequest") {
				json::Object request;
				std::istringstream ist(req.getPayload());
				json::Reader::Read(request,ist);
				json::Object response;

				std::string id = json::String(request["id"]).Value();
				pthread_mutex_lock(&m_tokenMapMutex);
				Userlevel::UserlevelToken* &token = m_tokens[id];
				pthread_mutex_unlock(&m_tokenMapMutex);

				try {
					std::string req = json::String(request["type"]).Value();
					if(req == "getAll") {
						getAll(response);
					} else if(req == "toggleOutput") {
						toggleOutput(token,request,response);
					} else if(req == "activateInput") {
						activateInput(token,request,true,response);
					} else if(req == "deactivateInput") {
						activateInput(token,request,false,response);
					} else if(req == "getAlarmState") {
						getAlarmState(response);
					} else if(req == "keypadAction") {
						keypadAction(token,request,response);
					} else if(req == "getModuleConfiguration") {
						getModuleConfiguration(token,request,response);
					} else if(req == "setModuleConfiguration") {
						setModuleConfiguration(token,request,response);
					} else if(req == "getUserlevel") {
						getUserlevel(token,request,response);
					}
					std::ostringstream ost;
					json::Writer::Write(response,ost);
					std::ostringstream header;
					header << "HTTP/1.1 200 OK\r\n";
					header << "Content-Length: " << ost.str().size() << "\r\n";
					header << "Content-type: application/json" << "\r\n";
					header << "Connection: close\r\n\r\n";
					m_socket->send(header.str());
					m_socket->send(ost.str());
				} catch (bool ex) {
					std::ostringstream header;
					header << "HTTP/1.1 403 Forbidden\r\n";
					header << "Content-Length: 0\r\n";
					header << "Connection: close\r\n\r\n";
					m_socket->send(header.str());
				}
			} else if (requestURI == "/ihcevents-ws" && handleWebSocketHandshake(req.getHeader())) {
				webSocketEventHandler();
			} else {
				std::string URI = req.getRequestURI();
				if(URI.find("/") != 0) {
					URI.insert(0,"/");
				}
				if(URI == "/") {
					URI = "/index.html";
				}
				URI.insert(0,webroot);
				std::ifstream file;
				file.open(URI.c_str(), (std::ios::in | std::ios::binary | std::ios::ate));
				if(file.is_open()) {
					unsigned int size = 0;
					size = file.tellg();
					char* memblock = new char [size];
					file.seekg (0, std::ios::beg);
					file.read (memblock, size);
					file.close();
					std::ostringstream header;
					header << "HTTP/1.1 200 OK\r\n";
					header << "Content-Length: " << size << "\r\n";
					header << "Connection: close\r\n\r\n";
					m_socket->send(header.str());
					m_socket->send((unsigned char*)memblock,size);
					delete[] memblock;
				} else {
					printf("Could not open %s\n",URI.c_str());
					std::ostringstream header;
					header << "HTTP/1.1 404 Not Found\r\n";
					header << "Content-Length: 0\r\n";
					header << "Connection: close\r\n\r\n";
					m_socket->send(header.str());
				}
			}
		} else if(buffer == "" && m_socket->peek() <= 0) {
//			break;
		}
	} catch (std::exception& ex) {
		printf("IHCHTTPServerWorker: Exception in packet (%s)\n",ex.what());
	} catch (bool ex) {
		printf("IHCHTTPServerWorker: Unknown exception in packet\n");
	}
}

bool IHCHTTPServerWorker::handleWebSocketHandshake(const std::string& header) {
	bool r = false;
//	printf("Header .%s.\n",header.c_str());
	const std::string versionString = "Sec-WebSocket-Version: ";
	const std::string keyString = "Sec-WebSocket-Key: ";

//	size_t p = header.find(versionString);
//	if(p != std::string::npos) {
//		size_t p2 = header.find("\r\n",p);
//		if(p2 != std::string::npos) {
//			printf("version %s.\n",header.substr(p+versionString.size(),p2-p-versionString.size()).c_str());
//		}
//	}
	std::string key = "";
	size_t p = header.find(keyString);
	if(p != std::string::npos) {
		size_t p2 = header.find("\r\n",p);
		if(p2 != std::string::npos) {
			key = header.substr(p+keyString.size(),p2-p-keyString.size());
//			printf("key %s.\n",key.c_str());
			const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			std::string c(key+GUID);
			const int hashlen = 20;
			unsigned char hash[hashlen];
			SHA1((const unsigned char*)c.c_str(), c.size(), hash);
			std::string acceptKey = base64_encode(hash,hashlen);
//			printf("acceptKey %s\n",acceptKey.c_str());

			std::ostringstream header;
			header << "HTTP/1.1 101 Switching Protocols\r\n";
			header << "Upgrade: websocket\r\n";
			header << "Connection: Upgrade\r\n";
			header << "Sec-WebSocket-Accept: " << acceptKey << "\r\n";
			header << "\r\n";
			m_socket->send(header.str());
			r = true;
		}
	}
	return r;
}

void IHCHTTPServerWorker::webSocketEventHandler() {
	m_eventMutex = new pthread_mutex_t;
	pthread_mutex_init(m_eventMutex,NULL);

        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	m_eventCond = new pthread_cond_t;
	pthread_cond_init(m_eventCond,&attr);
        pthread_condattr_destroy(&attr);

	m_ihcServer->attach(this);

	try {
		while(true) {
			IHCEvent* e = NULL;
			pthread_cleanup_push((void(*)(void*))pthread_mutex_unlock,(void*)m_eventMutex);
			pthread_mutex_lock(m_eventMutex);
			while(m_events.empty()) {
//				pthread_cond_wait(m_eventCond,m_eventMutex);
				struct timespec ts = ms::getAbsTime(ms::get()+5000);
				int rv = pthread_cond_timedwait(m_eventCond,m_eventMutex,&ts);
 				if(rv == ETIMEDOUT) {
					pingWebSocket();
				}
			}
			e = m_events.front();
			m_events.pop_front();
			pthread_mutex_unlock(m_eventMutex);
			pthread_cleanup_pop(0);

			json::Object response;

			switch(e->m_event) {
				case IHCServerDefs::ALARM_ARMED:
					response["type"] = json::String("alarmState");
					response["state"] = json::Boolean(true);
				break;
				case IHCServerDefs::ALARM_DISARMED:
					response["type"] = json::String("alarmState");
					response["state"] = json::Boolean(false);
				break;
				case IHCServerDefs::INPUT_CHANGED:
					response["type"] = json::String("inputState");
					response["moduleNumber"] = json::Number(e->m_io->getModuleNumber());
					response["ioNumber"] = json::Number(e->m_io->getIONumber());
					response["state"] = json::Boolean(e->m_io->getState());
				break;
				case IHCServerDefs::OUTPUT_CHANGED:
					response["type"] = json::String("outputState");
					response["moduleNumber"] = json::Number(e->m_io->getModuleNumber());
					response["ioNumber"] = json::Number(e->m_io->getIONumber());
					response["state"] = json::Boolean(e->m_io->getState());
				break;
			}
			response["lastEventNumber"] = json::Number(e->getEventNumber());

			delete e;

			std::ostringstream ost;
			json::Writer::Write(response,ost);
			unsigned char header[2];
			header[0] = 129;
			header[1] = ost.str().size();
			try {
				m_socket->send(header,2);
				m_socket->send(ost.str());
			} catch (bool ex) {
				throw ex;
			}
		}
	} catch (std::exception& ex) {
		printf("IHCHTTPServerWorker: WebSocketEventHandler caught exception (%s)\n",ex.what());
	} catch (bool ex) {
		printf("IHCHTTPServerWorker: WebSocketEventHandler caught exception\n");
	}

	m_ihcServer->detach(this);
	pthread_cond_destroy(m_eventCond);
	delete m_eventCond;
	m_eventCond = NULL;

	pthread_mutex_destroy(m_eventMutex);
	delete m_eventMutex;
	m_eventMutex = NULL;
}

bool IHCHTTPServerWorker::pingWebSocket() {
	json::Object response;
	std::ostringstream ost;
	response["type"] = json::String("ping");
	json::Writer::Write(response,ost);
	unsigned char header[2];
	header[0] = 129;
	header[1] = ost.str().size();
	try {
		m_socket->send(header,2);
		m_socket->send(ost.str());
		unsigned int start_ms = ms::get();
		while(m_socket->poll(1000)) {
			if(ms::isPast(start_ms + 3000)) {
				throw false;
			}
		};
		std::string buffer = "";
		while(m_socket->peek() > 0) {
			std::string buf = "";
			m_socket->recv(buf);
			buffer += buf;
			usleep(500*1000);
		}
		json::Object request;
		std::string r = decodeWebSocketPacket((const unsigned char*)(buffer.c_str()),buffer.size());
		if(r != "") {
			std::istringstream ist(r);
			json::Reader::Read(request,ist);
			if(json::String(request["type"]).Value() != "pong") {
				throw false;
			}
		} else {
			throw false;
		}
	} catch (bool ex) {
		throw ex;
	}
	return true;
}

std::string IHCHTTPServerWorker::decodeWebSocketPacket(const unsigned char* packet, unsigned int length) {
	std::string r = "";
	if(packet[0] == 0x81) {
		if((packet[1] & 0x80) == 0x80) {
			unsigned char lengthByte = (packet[1] & 0x7F);
			unsigned int maskStartOffset = 2;
			if(length == 0x7E) { // Next two bytes are length
				maskStartOffset = 4;
			} else if (length == 0x7F) { // Next eight bytes are length
				maskStartOffset = 10;
			}
			unsigned int dataStartOffset = maskStartOffset + 4;
			for(unsigned int k = dataStartOffset, j = 0; k < length; ++k,++j) {
				r += packet[k] ^ packet[maskStartOffset + (j % 4)];
			}
		}
	}
	return r;
}

void IHCHTTPServerWorker::update(Subject* sub, void* obj) {
	if(sub == m_ihcServer) {
		pthread_mutex_lock(m_eventMutex);
		m_events.push_back(new IHCEvent(*((IHCEvent*)obj)));
		pthread_cond_signal(m_eventCond);
		pthread_mutex_unlock(m_eventMutex);
	}
}

