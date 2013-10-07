#ifndef IHCHTTPSERVER_H
#define IHCHTTPSERVER_H
#include "utils/TCPSocketServer.h"
#include "utils/Observer.h"
#include <list>

class IHCEvent;

class IHCHTTPServer : public TCPSocketServer, public Observer {
public:
	static IHCHTTPServer* getInstance();
	virtual ~IHCHTTPServer();
	void clientConnected(TCPSocket* newSocket);
	void update(Subject* sub, void* obj);
private:
        pthread_mutex_t m_eventMutex;
        pthread_cond_t m_eventCond;
	static pthread_mutex_t m_instanceMutex;
	static IHCHTTPServer* m_instance;
	IHCHTTPServer();
	std::list<IHCEvent*> m_ihcEvents;
	static const int m_maxQueueSize = 20;
	friend class IHCHTTPServerWorker;
};

#endif /* IHCHTTPSERVER_H */
