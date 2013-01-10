#ifndef IHCSERVERWORKER_H
#define IHCSERVERWORKER_H
#include "utils/Subject.h"
#include "utils/Thread.h"

class TCPSocket;
class IHCServer;

class IHCServerWorker : public Thread, public Subject {
public:
	IHCServerWorker(TCPSocket* socket, IHCServer* server);
	void thread();
private:
	TCPSocket* m_socket;
	IHCServer* m_server;
};

#endif /* IHCSERVERWORKER_H */
