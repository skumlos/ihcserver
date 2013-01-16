/**
 * This is the main server application for the IHC system.
 * This class owns an instance of the IHCInterface that
 * it uses to serve the clients with requests and events.
 * It owns the two TCPSocketServers that clients connect to
 * depending on what they want.
 *
 * 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCSERVER_H
#define IHCSERVER_H
#include "utils/Observer.h"
#include "utils/Thread.h"
#include "utils/TCPSocketServerCallback.h"
#include "IHCServerDefs.h"
#include <list>

class IHCInterface;
class TCPSocketServer;
class IHCServerWorker;
class IHCServerEventWorker;
class Configuration;
class Reaper;
class IHCIO;

class IHCServer : public Thread, public Observer, public TCPSocketServerCallback {
public:
	IHCServer();
	~IHCServer();
	void thread();
	void socketConnected(TCPSocket* newSocket);
	void deleteServerWorker(IHCServerWorker* worker);

	void update(Subject* sub, void* obj);
	bool getInputState(int moduleNumber, int inputNumber);
	bool getOutputState(int moduleNumber, int outputNumber);
	void setOutputState(int moduleNumber, int outputNumber, bool state);

	void toggleModuleState(enum IHCServerDefs::Type type, int moduleNumber);
	bool getModuleState(enum IHCServerDefs::Type type, int moduleNumber);

	void setIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, std::string description);
	std::string getIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);

	void saveConfiguration();
private:

	// The interface to the IHC controller
	IHCInterface* m_ihcinterface;

	// Listeners that want to know about changes should register here
	TCPSocketServer* m_eventServer;
	std::list<IHCServerWorker*> m_eventListeners;
	std::list<IHCIO*> m_eventList;
	pthread_cond_t m_eventCond;
	pthread_mutex_t m_eventMutex;

	// This is for external requests to the server
	TCPSocketServer* m_requestServer;

	Configuration* m_configuration;

	const static int m_requestServerPort = 45200;
	const static int m_eventServerPort = 45201;
};

#endif /* IHCSERVER_H */
