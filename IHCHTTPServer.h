#ifndef IHCHTTPSERVER_H
#define IHCHTTPSERVER_H
#include "utils/TCPSocketServer.h"
#include "utils/Observer.h"
#include <list>

class IHCEvent;

class IHCHTTPServer : public TCPSocketServer {
public:
	static IHCHTTPServer* getInstance();
	virtual ~IHCHTTPServer();
	void clientConnected(TCPSocket* newSocket);
private:
	static pthread_mutex_t m_instanceMutex;
	static IHCHTTPServer* m_instance;
	IHCHTTPServer(int port);
	friend class IHCHTTPServerWorker;
};

#endif /* IHCHTTPSERVER_H */
