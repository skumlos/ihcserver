#ifndef IHCSERVER_H
#define IHCSERVER_H
#include "utils/Observer.h"
#include "utils/Thread.h"
#include "utils/TCPSocketServerCallback.h"
#include <list>

class IHCInterface;
class TCPSocketServer;
class IHCServerWorker;

class IHCServer : public Thread, public Observer, public TCPSocketServerCallback {
public:
	IHCServer();
	~IHCServer();
	void thread();
	void socketConnected(TCPSocket* newSocket);
	void update(Subject* sub, void* obj);
	bool getInputState(int moduleNumber, int inputNumber);
	bool getOutputState(int moduleNumber, int outputNumber);
private:
	// The interface to the IHC controller
	IHCInterface* m_ihcinterface;

	// Listeners that want to know about changes should register here
	TCPSocketServer* m_eventServer;

	// Used for the server to propagate I/O events
	TCPSocketServer* m_requestServer;

	const static int m_requestServerPort = 45200;
	const static int m_eventServerPort = 45201;
};

#endif /* IHCSERVER_H */
