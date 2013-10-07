#ifndef IHCHTTPSERVERWORKER_H
#define IHCHTTPSERVERWORKER_H
#include "utils/Thread.h"
#include "3rdparty/cajun-2.0.2/json/elements.h"
#include "Userlevel.h"
#include <map>

class TCPSocket;
class IHCEvent;

class IHCHTTPServerWorker : public Thread {
public:
	IHCHTTPServerWorker(TCPSocket* connectedSocket);
	virtual ~IHCHTTPServerWorker();
	void thread();
private:
	void getAll(json::Object& resp);
	void getAlarmState(json::Object& response);
	void keypadAction(json::Object& req, json::Object& response);
	void toggleOutput(json::Object& req, json::Object& response);
	void getModuleConfiguration(json::Object& req, json::Object& resp);
	void setModuleConfiguration(json::Object& req, json::Object& resp);
	static pthread_mutex_t m_tokenMapMutex;
	static std::map<std::string,Userlevel::UserlevelToken*> m_tokens;
	TCPSocket* m_socket;
};

#endif /* IHCHTTPSERVERWORKER_H */
