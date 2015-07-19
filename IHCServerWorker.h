/**
 * Copyright (c) 2013, Martin Hejnfelt (martin@hejnfelt.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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

        std::string m_clientID;
	IHCServer* m_server;

        pthread_mutex_t m_socketMutex;
        pthread_cond_t m_socketCond;
};

#endif /* IHCSERVERWORKER_H */
