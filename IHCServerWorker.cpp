#include "IHCServerWorker.h"
#include "IHCServer.h"
#include "utils/TCPSocket.h"

IHCServerWorker::IHCServerWorker(std::string clientID, TCPSocket* socket, IHCServer* server) :
	Thread(true),
	m_clientID(clientID),
	m_socket(socket),
	m_server(server)
{
	pthread_mutex_init(&m_socketMutex,NULL);
	pthread_cond_init(&m_socketCond,NULL);
}

IHCServerWorker::~IHCServerWorker() {
	pthread_cond_destroy(&m_socketCond);
	pthread_mutex_destroy(&m_socketMutex);
	delete m_socket;
}

void IHCServerWorker::doCleanup() {
}

void IHCServerWorker::setSocket(TCPSocket* newSocket) {
	pthread_mutex_lock(&m_socketMutex);
	delete m_socket;
	m_socket = newSocket;
	pthread_cond_signal(&m_socketCond);
	pthread_mutex_unlock(&m_socketMutex);
}
