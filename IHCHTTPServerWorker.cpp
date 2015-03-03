#include "IHCHTTPServerWorker.h"
#include "utils/TCPSocket.h"
#include "HTTPRequest.h"
#include "Configuration.h"
#include "IHCServer.h"
#include "IHCServerDefs.h"
#include "IHCEvent.h"
#include "IHCIO.h"
#include "IHCHTTPServer.h"
#include <cstdio>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"
pthread_mutex_t IHCHTTPServerWorker::m_tokenMapMutex = PTHREAD_MUTEX_INITIALIZER;
std::map<std::string,Userlevel::UserlevelToken*> IHCHTTPServerWorker::m_tokens;

IHCHTTPServerWorker::IHCHTTPServerWorker(TCPSocket* connectedSocket) :
	Thread(true),
	m_socket(connectedSocket)
{	start();
}

IHCHTTPServerWorker::~IHCHTTPServerWorker() {
	delete m_socket;
}

void IHCHTTPServerWorker::thread() {
	std::string webroot = Configuration::getInstance()->getWebroot();
	while(m_socket->isOpen()) {
		try {
			while(m_socket->poll(1000)) { sleep(1); };
				std::string buffer = "";
				std::string buf = "";
				while(m_socket->peek() > 0) {
					m_socket->recv(buf);
					buffer += buf;
					usleep(100*1000);
				}
				if(buffer != "") {
					HTTPRequest req(buffer);
//					printf("Buffer:\n%s\n",buffer.c_str());
					printf("Request type %d, URI: %s. Payload: %s.\n",req.getRequestType(),req.getRequestURI().c_str(),req.getPayload().c_str());
					std::string response = "";
					std::string requestURI = req.getRequestURI();
					if(requestURI == "/ihcrequest") {
						json::Object request;
						std::istringstream ist(req.getPayload());
						json::Reader::Read(request,ist);
						json::Object response;
						try {
							std::string req = json::String(request["type"]).Value();
							if(req == "getAll") {
								getAll(response);
							} else if(req == "toggleOutput") {
								toggleOutput(request,response);
							} else if(req == "activateInput") {
								activateInput(request,true,response);
							} else if(req == "deactivateInput") {
								activateInput(request,false,response);
							} else if(req == "getAlarmState") {
								getAlarmState(response);
							} else if(req == "keypadAction") {
								keypadAction(request,response);
							} else if(req == "getModuleConfiguration") {
								getModuleConfiguration(request,response);
							} else if(req == "setModuleConfiguration") {
								setModuleConfiguration(request,response);
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
					} else if (requestURI == "/ihcevents") {
						int lastEventNumber = 0;
						json::Object response;
						if(req.getPayload() != "") {
							json::Object request;
							std::istringstream ist(req.getPayload());
							json::Reader::Read(request,ist);
							try {
								lastEventNumber = json::Number(request["lastEventNumber"]);
							} catch(...) {
								printf("Could not get last event number\n");
							}
						}
						IHCHTTPServer* ihcHttpServer = IHCHTTPServer::getInstance();
						pthread_mutex_lock(&ihcHttpServer->m_eventMutex);
						if(lastEventNumber == 0 || lastEventNumber > ihcHttpServer->m_ihcEvents.front()->getEventNumber()) {
							lastEventNumber = ihcHttpServer->m_ihcEvents.front()->getEventNumber();
						}
						IHCEvent* ihcEvent = NULL;
						if(lastEventNumber != ihcHttpServer->m_ihcEvents.front()->getEventNumber()) {
							std::list<IHCEvent*>::iterator it = ihcHttpServer->m_ihcEvents.begin();
							for(; it != ihcHttpServer->m_ihcEvents.end(); it++) {
								if((*it)->getEventNumber() == lastEventNumber) {
									it--;
									ihcEvent = (*it);
									break;
								}
							}
							if(it == ihcHttpServer->m_ihcEvents.end()) {
								ihcEvent = ihcHttpServer->m_ihcEvents.back();
							}
						} else {
							while(ihcHttpServer->m_ihcEvents.front()->getEventNumber() == lastEventNumber) {
								pthread_cond_wait(&ihcHttpServer->m_eventCond,&ihcHttpServer->m_eventMutex);
							}
							ihcEvent = ihcHttpServer->m_ihcEvents.front();
						}
						if(ihcEvent != NULL) {
						switch(ihcEvent->m_event) {
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
								response["moduleNumber"] = json::Number(ihcEvent->m_io->getModuleNumber());
								response["ioNumber"] = json::Number(ihcEvent->m_io->getIONumber());
								response["state"] = json::Boolean(ihcEvent->m_io->getState());
							break;
							case IHCServerDefs::OUTPUT_CHANGED:
								response["type"] = json::String("outputState");
								response["moduleNumber"] = json::Number(ihcEvent->m_io->getModuleNumber());
								response["ioNumber"] = json::Number(ihcEvent->m_io->getIONumber());
								response["state"] = json::Boolean(ihcEvent->m_io->getState());
							break;
						}
						response["lastEventNumber"] = json::Number(ihcEvent->getEventNumber());
						pthread_mutex_unlock(&ihcHttpServer->m_eventMutex);
						std::ostringstream ost;
						json::Writer::Write(response,ost);
						std::ostringstream header;
						header << "HTTP/1.1 200 OK\r\n";
						header << "Content-Length: " << ost.str().size() << "\r\n";
						header << "Content-type: application/json" << "\r\n";
						header << "Connection: close\r\n\r\n";
						m_socket->send(header.str());
						m_socket->send(ost.str());
						}
					} else {
						std::string URI = req.getRequestURI();
						if(URI.find("/") != 0) {
							URI.insert(0,"/");
						}
						if(URI == "/") {
							URI = "/index2.html";
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
					break;
				}
		} catch (...) {
			printf("Exception in IHCHTTPServerWorker\n");
		}
	}
}
void IHCHTTPServerWorker::getModuleConfiguration(json::Object& req, json::Object& resp)
{
	IHCServer* ihcserver = IHCServer::getInstance();
	std::string id = json::String(req["id"]).Value();
	pthread_mutex_lock(&m_tokenMapMutex);
	Userlevel::UserlevelToken* &token = m_tokens[id]; 
	pthread_mutex_unlock(&m_tokenMapMutex);
	if(Userlevel::getUserlevel(token) != Userlevel::ADMIN) {
		printf("%s trying to getModuleConfiguration without proper level\n",id.c_str());
		throw false;
	}
	std::string moduleType = json::String(req["moduleType"]).Value();
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	resp["type"] = json::String("moduleConfiguration");
	resp["moduleNumber"] = json::Number(moduleNumber);
	resp["moduleType"] = json::String(moduleType);
	enum IHCServerDefs::Type type;
	int ios = 0;
	if(moduleType == "input") {
		type = IHCServerDefs::INPUTMODULE;
		ios = 16;
	} else if(moduleType == "output") {
		type = IHCServerDefs::OUTPUTMODULE;
		ios = 8;
	} else {
		throw false;
	}
	resp["state"] = json::Boolean(ihcserver->getModuleState(type,moduleNumber));
	json::Array ioDescriptions;
	for(int j = 1; j <= ios; j++) {
		json::Object ioDescription;
		if(ihcserver->getIODescription(type,moduleNumber,j) != "") {
			ioDescription["ioNumber"] = json::Number(j);
			ioDescription["ioDescription"] = json::String(ihcserver->getIODescription(type,moduleNumber,j));
			ioDescriptions.Insert(ioDescription);
		}
	}
	resp["ioDescriptions"] = ioDescriptions;
	json::Array ioProtectedStates;
	for(int j = 1; j <= ios; j++) {
		json::Object ioProtected;
		if(ihcserver->getIOProtected(type,moduleNumber,j)) {
			ioProtected["ioNumber"] = json::Number(j);
			ioProtected["ioProtected"] = json::Boolean(true);
			ioProtectedStates.Insert(ioProtected);
		}
	}
	resp["ioProtectedStates"] = ioProtectedStates;
	json::Array ioAlarmStates;
	for(int j = 1; j <= ios; j++) {
		json::Object ioAlarm;
		if(ihcserver->getIOAlarm(type,moduleNumber,j)) {
			ioAlarm["ioNumber"] = json::Number(j);
			ioAlarm["ioAlarm"] = json::Boolean(true);
			ioAlarmStates.Insert(ioAlarm);
		}
	}
	resp["ioAlarmStates"] = ioAlarmStates;
	return;
}

void IHCHTTPServerWorker::setModuleConfiguration(json::Object& req, json::Object& resp)
{
	IHCServer* ihcserver = IHCServer::getInstance();
	std::string id = json::String(req["id"]).Value();
	pthread_mutex_lock(&m_tokenMapMutex);
	Userlevel::UserlevelToken* &token = m_tokens[id];
	pthread_mutex_unlock(&m_tokenMapMutex);
	if(Userlevel::getUserlevel(token) != Userlevel::ADMIN) {
		printf("%s trying to setModuleConfiguration without proper level\n",id.c_str());
		throw false;
	}
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	std::string moduleType = json::String(req["moduleType"]).Value();
	enum IHCServerDefs::Type type;
	int ios = 0;
	if(moduleType == "input") {
		type = IHCServerDefs::INPUTMODULE;
	} else if(moduleType == "output") {
		type = IHCServerDefs::OUTPUTMODULE;
	} else {
		throw false;
	}

	json::Array::const_iterator it;
	json::Array ioDescriptions = json::Array(req["ioDescriptions"]);
	for(it = ioDescriptions.Begin(); it != ioDescriptions.End(); it++) {
		json::Object ioDescriptionObj = json::Object(*it);
		int ioNumber = json::Number(ioDescriptionObj["ioNumber"]).Value();
		std::string ioDescription = json::String(ioDescriptionObj["ioDescription"]).Value();
		if(ioDescription != "") {
			ihcserver->setIODescription(type,moduleNumber,ioNumber,ioDescription);
		}
	}
	json::Array ioProtectedStates = json::Array(req["ioProtectedStates"]);
	for(it = ioProtectedStates.Begin(); it != ioProtectedStates.End(); it++) {
		json::Object ioProtectedObj = json::Object(*it);
		int ioNumber = json::Number(ioProtectedObj["ioNumber"]).Value();
		bool ioProtected = json::Boolean(ioProtectedObj["ioProtected"]).Value();
		ihcserver->setIOProtected(type,moduleNumber,ioNumber,ioProtected);
	}
	json::Array ioAlarmStates = json::Array(req["ioAlarmStates"]);
	for(it = ioAlarmStates.Begin(); it != ioAlarmStates.End(); it++) {
		json::Object ioAlarmObj = json::Object(*it);
		int ioNumber = json::Number(ioAlarmObj["ioNumber"]).Value();
		bool ioAlarm = json::Boolean(ioAlarmObj["ioAlarm"]).Value();
		ihcserver->setIOAlarm(type,moduleNumber,ioNumber,ioAlarm);
	}
	bool moduleState = json::Boolean(req["state"]).Value();
	if(moduleState != ihcserver->getModuleState(type,moduleNumber)) {
		ihcserver->toggleModuleState(type,moduleNumber);
	}
	ihcserver->saveConfiguration();
	resp["result"] = json::Boolean(true);
	return;
}

void IHCHTTPServerWorker::toggleOutput(json::Object& req, json::Object& response) {
	IHCServer* ihcserver = IHCServer::getInstance();
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	int outputNumber = json::Number(req["ioNumber"]).Value();
	std::string id = json::String(req["id"]).Value();
	pthread_mutex_lock(&m_tokenMapMutex);
	Userlevel::UserlevelToken* &token = m_tokens[id];
	pthread_mutex_unlock(&m_tokenMapMutex);
	if(ihcserver->getIOProtected(IHCServerDefs::OUTPUTMODULE,moduleNumber,outputNumber) &&
	   (Userlevel::getUserlevel(token) != Userlevel::ADMIN && Userlevel::getUserlevel(token) != Userlevel::SUPERUSER))
	{
		throw false;
	}
	bool state = ihcserver->getOutputState(moduleNumber,outputNumber);
	ihcserver->setOutputState(moduleNumber,outputNumber,!state);
	response["type"] = json::String("outputState");
	response["moduleNumber"] = json::Number(moduleNumber);
	response["outputNumber"] = json::Number(outputNumber);
	response["state"] = json::Boolean(state);
	return;
}

void IHCHTTPServerWorker::activateInput(json::Object& req, bool shouldActivate, json::Object& response) {
	IHCServer* ihcserver = IHCServer::getInstance();
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	int outputNumber = json::Number(req["ioNumber"]).Value();
	std::string id = json::String(req["id"]).Value();
	pthread_mutex_lock(&m_tokenMapMutex);
	Userlevel::UserlevelToken* &token = m_tokens[id];
	pthread_mutex_unlock(&m_tokenMapMutex);
/*	if(ihcserver->getIOProtected(IHCServerDefs::OUTPUTMODULE,moduleNumber,outputNumber) &&
	   (Userlevel::getUserlevel(token) != Userlevel::ADMIN && Userlevel::getUserlevel(token) != Userlevel::SUPERUSER))
	{
		throw false;
	}*/
	ihcserver->activateInput(moduleNumber,outputNumber,shouldActivate);
	return;
}

void IHCHTTPServerWorker::getAlarmState(json::Object& response) {
	IHCServer* ihcserver = IHCServer::getInstance();
	response["type"] = json::String("getAlarmState");
	response["state"] = json::Boolean(ihcserver->getAlarmState());
	return;
}

void IHCHTTPServerWorker::getAll(json::Object& resp) {
	IHCServer* ihcserver = IHCServer::getInstance();
	resp["type"] = json::String("allModules");
	json::Object modules;
	json::Array inputModules;
	for(unsigned int j = 1; j <= 8; j++) {
		json::Object module;
		module["moduleNumber"] = json::Number(j);
		bool state = ihcserver->getModuleState(IHCServerDefs::INPUTMODULE,j);
		module["state"] = json::Boolean(state);
		if(state) {
			json::Array inputStates;
			for(unsigned int k = 1; k <= 16; k++) {
				json::Object inputState;
				inputState["inputNumber"] = json::Number(k);
				inputState["inputState"] = json::Boolean(ihcserver->getInputState(j,k));
				inputState["description"] = json::String(ihcserver->getIODescription(IHCServerDefs::INPUTMODULE,j,k));
				inputState["protected"] = json::Boolean(ihcserver->getIOProtected(IHCServerDefs::INPUTMODULE,j,k));
				inputStates.Insert(inputState);
			}
			module["inputStates"] = inputStates;
		}
		inputModules.Insert(module);
	}
	modules["inputModules"] = inputModules;
	json::Array outputModules;
	for(unsigned int j = 1; j <= 16; j++) {
		json::Object module;
		module["moduleNumber"] = json::Number(j);
		bool state = ihcserver->getModuleState(IHCServerDefs::OUTPUTMODULE,j);
		module["state"] = json::Boolean(state);
		if(state) {
			json::Array outputStates;
			for(unsigned int k = 1; k <= 8; k++) {
				json::Object outputState;
				outputState["outputNumber"] = json::Number(k);
				outputState["outputState"] = json::Boolean(ihcserver->getOutputState(j,k));
				outputState["description"] = json::String(ihcserver->getIODescription(IHCServerDefs::OUTPUTMODULE,j,k));
				outputState["protected"] = json::Boolean(ihcserver->getIOProtected(IHCServerDefs::OUTPUTMODULE,j,k));
				outputStates.Insert(outputState);
			}
			module["outputStates"] = outputStates;
		}
		outputModules.Insert(module);
	}
	modules["outputModules"] = outputModules;
	resp["modules"] = modules;
	return;
}

void IHCHTTPServerWorker::keypadAction(json::Object& req, json::Object& response)
{
	std::string action = json::String(req["action"]).Value();
	std::string id = json::String(req["id"]).Value();
	std::string input = json::String(req["input"]).Value();
	pthread_mutex_lock(&m_tokenMapMutex);
	Userlevel::UserlevelToken* &token = m_tokens[id]; 
	pthread_mutex_unlock(&m_tokenMapMutex);
	if(action == "setAdminCode") {
		if(input != "") {
			if(Userlevel::getUserlevel(token) == Userlevel::ADMIN) {
				Userlevel::setCodeSHA(Userlevel::ADMIN,input);
			}
		}
	} else if(action == "setSuperUserCode") {
		if(input != "") {
			if(Userlevel::getUserlevel(token) == Userlevel::SUPERUSER || 
				Userlevel::getUserlevel(token) == Userlevel::ADMIN) {
				Userlevel::setCodeSHA(Userlevel::SUPERUSER,input);
			}
		}
	} else if(action == "login") {
		try {
			Userlevel::loginSHA(token,input);
		} catch(...) {
			printf("Exception when logging in\n");
		}
	} else if(action == "arm-alarm") {
		Userlevel::UserlevelToken *tempToken;
		try {
			Userlevel::loginSHA(tempToken,input);
		} catch(...) {
			printf("Exception when logging in\n");
		}
		if(Userlevel::getUserlevel(tempToken) == Userlevel::BASIC) {
			printf("%s trying to arm without proper level\n",id.c_str());
			throw false;
		}
		IHCServer::getInstance()->setAlarmState(true);
	} else if(action == "disarm-alarm") {
		Userlevel::UserlevelToken *tempToken;
		try {
			Userlevel::loginSHA(tempToken,input);
		} catch(...) {
			printf("Exception when logging in\n");
		}
		if(Userlevel::getUserlevel(tempToken) == Userlevel::BASIC) {
			printf("%s trying to disarm without proper level\n",id.c_str());
			throw false;
		}
		IHCServer::getInstance()->setAlarmState(false);
	}
	response["result"] = json::Boolean(true);
}
