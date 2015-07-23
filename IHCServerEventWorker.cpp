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
#include "IHCEvent.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"

IHCServerEventWorker::IHCServerEventWorker(TCPSocket* socket) :
	IHCServerWorker(),
	m_socket(socket),
	m_ihcServer(IHCServer::getInstance())
{
	pthread_mutex_init(&m_eventMutex,NULL);
	pthread_cond_init(&m_eventCond,NULL);
	printf("IHCSeverEventWorker: Started for %s\n",m_socket->getHostname().c_str());
	start();
}

IHCServerEventWorker::~IHCServerEventWorker() {
	pthread_cond_destroy(&m_eventCond);
	pthread_mutex_destroy(&m_eventMutex);
	while(!m_events.empty()) {
		delete m_events.front();
		m_events.pop_front();
	}
}

void IHCServerEventWorker::thread() {
	// We dont want sigpipe, instead we ignore and TCPSocket will fire an
	// exception and make sure we get deleted
	signal(SIGPIPE,SIG_IGN);

	m_ihcServer->attach(this);
        try {
                while(true) {
			IHCEvent* e = NULL;
			pthread_cleanup_push((void(*)(void*))pthread_mutex_unlock,(void*)&m_eventMutex);
			pthread_mutex_lock(&m_eventMutex);
                        while(m_events.empty()) {
                              pthread_cond_wait(&m_eventCond,&m_eventMutex);
                        }
                        e = m_events.front();
                        m_events.pop_front();
                        pthread_mutex_unlock(&m_eventMutex);
                        pthread_cleanup_pop(0);

                        json::Object response;

                        switch(e->m_event) {
                                case IHCServerDefs::ALARM_ARMED:
                                        response["type"] = json::String("alarmState");
                                        response["state"] = json::Boolean(true);
                                break;
                                case IHCServerDefs::ALARM_DISARMED:
                                        response["type"] = json::String("alarmState");
                                        response["state"] = json::Boolean(false);
                                break;
                                case IHCServerDefs::INPUT_CHANGED:
                                        response["type"] = json::String("inputState");
                                        response["moduleNumber"] = json::Number(e->m_io->getModuleNumber());
                                        response["ioNumber"] = json::Number(e->m_io->getIONumber());
                                        response["state"] = json::Boolean(e->m_io->getState());
                                break;
                                case IHCServerDefs::OUTPUT_CHANGED:
                                        response["type"] = json::String("outputState");
                                        response["moduleNumber"] = json::Number(e->m_io->getModuleNumber());
                                        response["ioNumber"] = json::Number(e->m_io->getIONumber());
                                        response["state"] = json::Boolean(e->m_io->getState());
                                break;
                                case IHCServerDefs::ENTRY:
                                        response["type"] = json::String("entryEvent");
                                        response["moduleNumber"] = json::Number(e->m_io->getModuleNumber());
                                        response["ioNumber"] = json::Number(e->m_io->getIONumber());
                                break;
				case IHCServerDefs::UNKNOWN:
					continue;
				default:
				break;
                        }

			delete e;

			std::ostringstream ost;
			json::Writer::Write(response,ost);
			unsigned char sendHeader[4];
			size_t len = ost.str().size();
			sendHeader[0] = (unsigned char) (len >> 24) & 0xFF;
			sendHeader[1] = (unsigned char) (len >> 16) & 0xFF;
			sendHeader[2] = (unsigned char) (len >> 8) & 0xFF;
			sendHeader[3] = (unsigned char) (len >> 0) & 0xFF;

			m_socket->send(sendHeader,4);
			m_socket->send(ost.str());
		}
	} catch (std::exception& ex) {
		printf("IHCSeverEventWorker: Caught exception for %s (%s)\n",m_socket->getHostname().c_str(),ex.what());
		// Exception in socket, probably closed, bail out
	} catch (bool ex) {
		printf("IHCSeverEventWorker: Caught exception for %s\n",m_socket->getHostname().c_str());
	}
	m_ihcServer->detach(this);
}

void IHCServerEventWorker::update(Subject* sub, void* obj) {
        if(sub == m_ihcServer) {
                pthread_mutex_lock(&m_eventMutex);
                m_events.push_back(new IHCEvent(*((IHCEvent*)obj)));
                pthread_cond_signal(&m_eventCond);
                pthread_mutex_unlock(&m_eventMutex);
        }
}
