#include "IHCHTTPServer.h"
#include "IHCHTTPServerWorker.h"
#include "IHCServer.h"
#include "IHCEvent.h"
#include "IHCInput.h"
#include "IHCOutput.h"
#include "Configuration.h"
#include "IHCServerDefs.h"
#include <sstream>
#include <cstdio>

IHCHTTPServer* IHCHTTPServer::m_instance = NULL;
pthread_mutex_t IHCHTTPServer::m_instanceMutex = PTHREAD_MUTEX_INITIALIZER;

IHCHTTPServer* IHCHTTPServer::getInstance() {
	pthread_mutex_lock(&m_instanceMutex);
	if(m_instance == NULL) {
		Configuration* c = Configuration::getInstance();
		std::istringstream portStr(c->getValue(IHCServerDefs::HTTP_PORT_CONFKEY));
		int port = 0;
		portStr >> port;
		if(portStr.fail() || port < 80 || port > 65535) {
			port = 8081;
			std::ostringstream o;
			o << port;
			c->setValue(IHCServerDefs::HTTP_PORT_CONFKEY,o.str());
			c->save();
		}
		m_instance = new IHCHTTPServer(port);
	}
	pthread_mutex_unlock(&m_instanceMutex);
	return m_instance;
}

IHCHTTPServer::IHCHTTPServer(int port) :
	TCPSocketServer(port)
{
	printf("Starting HTTP Server on port %d\n",port);
	start();
}

IHCHTTPServer::~IHCHTTPServer() {}

void IHCHTTPServer::clientConnected(TCPSocket* newSocket) {
	new IHCHTTPServerWorker(newSocket);
}

