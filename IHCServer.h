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
#include "utils/Subject.h"
#include "utils/Thread.h"
#include "utils/TCPSocketServerCallback.h"
#include "IHCServerDefs.h"
#include <list>

class IHCInterface;
class TCPSocketServer;
class IHCHTTPServer;
class Configuration;
class Reaper;
class IHCEvent;
class IHCIO;

class IHCServer : public Thread, public Observer, public TCPSocketServerCallback, public Subject {
public:
	static IHCServer* getInstance();
	~IHCServer();
	void thread();

	// The callback that gets connected sockets from the servers
	void socketConnected(TCPSocket* newSocket, TCPSocketServer* caller);

	// We get notified by IHCInputs and IHCOutputs here
	void update(Subject* sub, void* obj);

	// Interface for the socketworkers
	bool getInputState(int moduleNumber, int inputNumber);
	bool getOutputState(int moduleNumber, int outputNumber);
	void setOutputState(int moduleNumber, int outputNumber, bool state);

	void activateInput(int moduleNumber, int inputNumber, bool shouldActivate);

	void toggleModuleState(enum IHCServerDefs::Type type, int moduleNumber);
	bool getModuleState(enum IHCServerDefs::Type type, int moduleNumber);

	void setIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, std::string description);
	std::string getIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);

	void setIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isProtected);
	bool getIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);

	void setIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isAlarm);
	bool getIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);

	void setIOEntry(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isEntry);
	bool getIOEntry(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);

	bool getAlarmState();
	void setAlarmState(bool alarmState);

	void saveConfiguration();

private:
	IHCServer();
	static pthread_mutex_t m_instanceMutex;
	static IHCServer* m_instance;

	// The interface to the IHC controller
	IHCInterface* m_ihcinterface;

	// Listeners that want to know about changes should register here
	TCPSocketServer* m_eventServer;
	const static int m_eventServerPort = 45201;
	std::list<IHCEvent*> m_eventList;

	pthread_cond_t m_eventCond;
	pthread_mutex_t m_eventMutex;
	pthread_mutex_t m_workerMutex;

	// This is for external requests to the server
	TCPSocketServer* m_requestServer;
	const static int m_requestServerPort = 45200;

	// The instance of the configuration
	Configuration* m_configuration;

	IHCHTTPServer* m_httpServer;

	bool m_alarmState;

	const static std::string version;
};

#endif /* IHCSERVER_H */
