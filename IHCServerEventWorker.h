#ifndef IHCSERVEREVENTWORKER_H
#define IHCSERVEREVENTWORKER_H
#include "IHCServerWorker.h"
#include "utils/Subject.h"
#include "utils/Thread.h"
#include "IHCServerDefs.h"
#include "3rdparty/cajun-2.0.2/json/elements.h"

class TCPSocket;
class IHCServer;

class IHCServerEventWorker : public IHCServerWorker {
public:
	IHCServerEventWorker(TCPSocket* socket, IHCServer* server);
	virtual ~IHCServerEventWorker();
	void thread();
	void notify(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, int state);
private:
	pthread_mutex_t m_messageMutex;
	pthread_cond_t m_messageCond;

	json::Object* m_message;
	TCPSocket* m_socket;
	IHCServer* m_server;
};

#endif /* IHCSERVEREVENTWORKER_H */
