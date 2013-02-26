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

class TCPSocket;
class IHCServer;

class IHCServerWorker : public Thread, public Subject {
public:
	IHCServerWorker(std::string clientID, TCPSocket* socket, IHCServer* server);

	virtual ~IHCServerWorker();

	virtual void thread() = 0;

	void doCleanup();

        std::string getClientID() { return m_clientID; };

	void setSocket(TCPSocket* newSocket);

protected:
        std::string m_clientID;
	TCPSocket* m_socket;
	IHCServer* m_server;

        pthread_mutex_t m_socketMutex;
        pthread_cond_t m_socketCond;

};

#endif /* IHCSERVERWORKER_H */
