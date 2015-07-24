/**
 * This class is an worker that waits for events and sends information about
 * these to the server formed as a JSON packet.
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCSERVEREVENTWORKER_H
#define IHCSERVEREVENTWORKER_H
#include "IHCServerWorker.h"
#include "utils/Observer.h"
#include "utils/Thread.h"
#include "IHCServerDefs.h"
#include "3rdparty/cajun-2.0.2/json/elements.h"
#include <list>

class TCPSocket;
class IHCServer;
class IHCEvent;

class IHCServerEventWorker : public IHCServerWorker, public Observer {
public:
	IHCServerEventWorker(TCPSocket* socket);
	virtual ~IHCServerEventWorker();
	void thread();
	void update(Subject* sub, void* obj);
private:
	TCPSocket* m_socket;
	IHCServer* m_ihcServer;
	std::list<IHCEvent*> m_events;
	pthread_mutex_t m_eventMutex;
	pthread_cond_t m_eventCond;
};

#endif /* IHCSERVEREVENTWORKER_H */
