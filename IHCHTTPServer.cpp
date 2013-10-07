#include "IHCHTTPServer.h"
#include "IHCHTTPServerWorker.h"
#include "IHCServer.h"
#include "IHCEvent.h"
#include "IHCInput.h"
#include "IHCOutput.h"
#include <cstdio>

IHCHTTPServer* IHCHTTPServer::m_instance = NULL;
pthread_mutex_t IHCHTTPServer::m_instanceMutex = PTHREAD_MUTEX_INITIALIZER;

IHCHTTPServer* IHCHTTPServer::getInstance() {
	pthread_mutex_lock(&m_instanceMutex);
	if(m_instance == NULL) {
		m_instance = new IHCHTTPServer();
	}
	pthread_mutex_unlock(&m_instanceMutex);
	return m_instance;
}

IHCHTTPServer::IHCHTTPServer() :
	TCPSocketServer(8081)
{
	printf("Starting HTTP Server on port 8081\n");
	pthread_mutex_init(&m_eventMutex,0);
	pthread_cond_init(&m_eventCond,0);
	start();
}

IHCHTTPServer::~IHCHTTPServer()
{
	IHCServer::getInstance()->detach(this);
	pthread_cond_destroy(&m_eventCond);
	pthread_mutex_destroy(&m_eventMutex);
}

void IHCHTTPServer::clientConnected(TCPSocket* newSocket) {
	new IHCHTTPServerWorker(newSocket);
}

void IHCHTTPServer::update(Subject* sub, void* obj) {
        if(sub == IHCServer::getInstance()) {
                IHCEvent* event = (IHCEvent*)obj;
                pthread_mutex_lock(&m_eventMutex);
                IHCEvent* ihcEvent = new IHCEvent(*event);
                if(dynamic_cast<IHCOutput*>(event->m_io) != 0) {
                        ihcEvent->m_io = new IHCOutput(*((IHCOutput*)event->m_io));
                } else if(dynamic_cast<IHCInput*>(event->m_io) != 0) {
                        ihcEvent->m_io = new IHCInput(*((IHCInput*)event->m_io));
                }
                m_ihcEvents.push_front(ihcEvent);
		if(m_ihcEvents.size() >= m_maxQueueSize) {
			delete m_ihcEvents.back();
			m_ihcEvents.pop_back();
		}
                pthread_cond_broadcast(&m_eventCond);
                pthread_mutex_unlock(&m_eventMutex);
        }
}

