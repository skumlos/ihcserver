#include "IHCServerEventWorker.h"
#include <string>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include <signal.h>
#include "IHCServer.h"
#include "utils/TCPSocket.h"
#include "IHCServerDefs.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"

IHCServerEventWorker::IHCServerEventWorker(TCPSocket* socket,IHCServer* server) :
	IHCServerWorker(socket,server),
	m_message(NULL),
	m_socket(socket),
	m_server(server)
{
	pthread_mutex_init(&m_messageMutex,NULL);
	pthread_cond_init(&m_messageCond,NULL);
	start();
}

IHCServerEventWorker::~IHCServerEventWorker() {
	pthread_cond_destroy(&m_messageCond);
	pthread_mutex_destroy(&m_messageMutex);
}

void IHCServerEventWorker::notify(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, int state) {
	pthread_mutex_lock(&m_messageMutex);
	m_message = new json::Object();
	if(type == IHCServerDefs::INPUT) {
		(*m_message)["type"] = json::String("inputState");
	} else if(type == IHCServerDefs::OUTPUT) {
		(*m_message)["type"] = json::String("outputState");
	} else {
		delete m_message;
		m_message = NULL;
		pthread_mutex_unlock(&m_messageMutex);
		return;
	}
	(*m_message)["moduleNumber"] = json::Number(moduleNumber);
	(*m_message)["ioNumber"] = json::Number(ioNumber);
	(*m_message)["state"] = json::Boolean(state);
	pthread_cond_signal(&m_messageCond);
	pthread_mutex_unlock(&m_messageMutex);
	return;
}

void IHCServerEventWorker::thread() {
	signal(SIGPIPE,SIG_IGN);
	try {
		while(1) {
			pthread_mutex_lock(&m_messageMutex);
			while(m_message == NULL) {
				pthread_cond_wait(&m_messageCond,&m_messageMutex);
			}
			pthread_mutex_unlock(&m_messageMutex);
			std::ostringstream ost;
			try {
				json::Writer::Write(*m_message,ost);
				int stringlength = ost.str().size();
				unsigned char* header = new unsigned char[4];
				header[0] = (unsigned char) (stringlength >> 0);
				header[1] = (unsigned char) (stringlength >> 8);
				header[2] = (unsigned char) (stringlength >> 16);
				header[3] = (unsigned char) (stringlength >> 24);
				m_socket->send(header,4);
				m_socket->send(ost.str());
			} catch (...) {
				throw false;
			}
			pthread_mutex_lock(&m_messageMutex);
			delete m_message;
			m_message = NULL;
			pthread_mutex_unlock(&m_messageMutex);
		}
	} catch (...) {
//		printf("IHCServerEventWorker: Exception in socket, probably closed\n");
	}
}

