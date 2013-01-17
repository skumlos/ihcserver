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
 * This class is an worker that waits for I/O changes and sends information about
 * these to the server formed as a JSON packet.
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

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
