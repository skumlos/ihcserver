/**
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCSERVERWORKER_H
#define IHCSERVERWORKER_H
#include "utils/Subject.h"
#include "utils/Thread.h"
#include <pthread.h>
#include <string>
#include "3rdparty/cajun-2.0.2/json/elements.h"
#include "Userlevel.h"

class IHCServer;

class IHCServerWorker : public Thread, public Subject {
public:
	IHCServerWorker();

	virtual ~IHCServerWorker();

	virtual void thread() = 0;

protected:
	void getAll(json::Object& resp);

	void getAlarmState(json::Object& response);

	void keypadAction(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& response);

	void toggleOutput(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& response);

	void activateInput(Userlevel::UserlevelToken* &token, json::Object& req, bool shouldActivate, json::Object& response);

	void getModuleConfiguration(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& resp);

	void setModuleConfiguration(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& resp);

	void getUserlevel(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& resp);

        std::string m_clientID;
	IHCServer* m_server;

        pthread_mutex_t m_socketMutex;
        pthread_cond_t m_socketCond;
};

#endif /* IHCSERVERWORKER_H */
