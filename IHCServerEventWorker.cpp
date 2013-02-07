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


#include "IHCServerEventWorker.h"
#include <string>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "IHCServer.h"
#include "utils/TCPSocket.h"
#include "utils/ms.h"
#include "IHCServerDefs.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"

IHCServerEventWorker::IHCServerEventWorker(std::string clientID, TCPSocket* socket,IHCServer* server) :
	m_clientID(clientID),
	IHCServerWorker(socket,server),
	m_socket(socket),
	m_server(server)
{
	pthread_mutex_init(&m_messageMutex,NULL);
	pthread_cond_init(&m_messageCond,NULL);
	pthread_mutex_init(&m_socketMutex,NULL);
	pthread_cond_init(&m_socketCond,NULL);
	start();
}

IHCServerEventWorker::~IHCServerEventWorker() {
	pthread_cond_destroy(&m_socketCond);
	pthread_mutex_destroy(&m_socketMutex);
	pthread_cond_destroy(&m_messageCond);
	pthread_mutex_destroy(&m_messageMutex);
	while(!m_messages.empty()) {
		json::Object* message = m_messages.front();
		m_messages.pop_front();
		delete message;
	}
}

void IHCServerEventWorker::notify(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, int state) {
	// We push the notification in to avoid the caller locking up or whatever during socket i/o
	pthread_mutex_lock(&m_messageMutex);
	json::Object* message = new json::Object();
	if(type == IHCServerDefs::INPUT) {
		(*message)["type"] = json::String("inputState");
	} else if(type == IHCServerDefs::OUTPUT) {
		(*message)["type"] = json::String("outputState");
	} else {
		delete message;
		message = NULL;
		pthread_mutex_unlock(&m_messageMutex);
		return;
	}
	(*message)["moduleNumber"] = json::Number(moduleNumber);
	(*message)["ioNumber"] = json::Number(ioNumber);
	(*message)["state"] = json::Boolean(state);
	m_messages.push_back(message);
	pthread_cond_signal(&m_messageCond);
	pthread_mutex_unlock(&m_messageMutex);
	return;
}

void IHCServerEventWorker::setSocket(TCPSocket* newSocket) {
	pthread_mutex_lock(&m_socketMutex);
	delete m_socket;
	m_socket = newSocket;
	pthread_cond_signal(&m_socketCond);
	pthread_mutex_unlock(&m_socketMutex);
}

void IHCServerEventWorker::thread() {
	// We dont want sigpipe, instead we ignore and TCPSocket will fire an
	// exception and make sure we get deleted
	signal(SIGPIPE,SIG_IGN);
	unsigned int minDelta_ms = 1000;
	try {
		json::Object* message = NULL;
		unsigned int lastSent_ms = 0;
		while(1) {
			std::ostringstream ost;
			try {
				pthread_mutex_lock(&m_messageMutex);
				while(m_messages.empty()) {
					pthread_cond_wait(&m_messageCond,&m_messageMutex);
				}
				// We have message(s)
				message = m_messages.front();
				json::Writer::Write(*message,ost);
				pthread_mutex_unlock(&m_messageMutex);
			} catch (...) {
				// Shit hit the fan, abort!
				pthread_mutex_unlock(&m_messageMutex);
				throw false;
			}
			try {
				int stringlength = ost.str().size();
				unsigned char* header = new unsigned char[4];
				header[0] = (unsigned char) (stringlength >> 0);
				header[1] = (unsigned char) (stringlength >> 8);
				header[2] = (unsigned char) (stringlength >> 16);
				header[3] = (unsigned char) (stringlength >> 24);

				pthread_mutex_lock(&m_socketMutex);
				m_socket->send(header,4);
				m_socket->send(ost.str());
				pthread_mutex_unlock(&m_socketMutex);

				lastSent_ms = ms::get();

				pthread_mutex_lock(&m_messageMutex);
 				m_messages.pop_front();
				pthread_mutex_unlock(&m_messageMutex);

				delete message;
				message = NULL;
			} catch (...) {
				struct timespec t;
				clock_gettime(CLOCK_REALTIME,&t);
				t.tv_sec += 10;
				int ret = pthread_cond_timedwait(&m_socketCond,&m_socketMutex,&t);
				pthread_mutex_unlock(&m_socketMutex);
				if(ret == ETIMEDOUT) {
					throw false;
				}
			}
			while(!ms::isPast(lastSent_ms+minDelta_ms)) usleep(10*1000);
		}
	} catch (...) {
		// Exception in socket, probably closed, bail out
	}
}

