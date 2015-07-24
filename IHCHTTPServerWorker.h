#ifndef IHCHTTPSERVERWORKER_H
#define IHCHTTPSERVERWORKER_H
#include "utils/Thread.h"
#include "utils/Observer.h"
#include "Userlevel.h"
#include "IHCServerWorker.h"
#include "3rdparty/cajun-2.0.2/json/elements.h"
#include <list>

class TCPSocket;
class IHCEvent;
class IHCServer;
class TokenInfo;

class IHCHTTPServerWorker : public IHCServerWorker, public Observer {
public:
	IHCHTTPServerWorker(TCPSocket* connectedSocket);
	virtual ~IHCHTTPServerWorker();
	void thread();
	void update(Subject* sub, void* obj);
private:
	static unsigned int m_all;
	static pthread_mutex_t m_allMutex;

	bool handleWebSocketHandshake(const std::string& header);
	bool pingWebSocket();
	void webSocketEventHandler();
	std::string decodeWebSocketPacket(const unsigned char* packet, unsigned int length);

	static pthread_mutex_t m_tokenMapMutex;
	static std::list<TokenInfo*> m_tokens;

	TCPSocket* m_socket;
	IHCServer* m_ihcServer;
	pthread_mutex_t* m_eventMutex;
	pthread_cond_t* m_eventCond;
	std::list<IHCEvent*> m_events;
};

#endif /* IHCHTTPSERVERWORKER_H */
