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


#include "IHCServerRequestWorker.h"
#include <string>
#include <unistd.h>
#include <signal.h>
#include <cstdio>
#include <sstream>
#include <stdint.h>
#include <errno.h>
#include "IHCServer.h"
#include "IHCServerDefs.h"
#include "utils/TCPSocket.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"
#include "Userlevel.h"

IHCServerRequestWorker::IHCServerRequestWorker(TCPSocket* socket) :
	IHCServerWorker(),
	m_socket(socket),
	m_token(0)
{
	printf("IHCServerRequestWorker: Started for %s\n",m_socket->getHostname().c_str());
	start();
}

IHCServerRequestWorker::~IHCServerRequestWorker() {
	delete m_socket;
}

void IHCServerRequestWorker::thread() {
	signal(SIGPIPE,SIG_IGN);
	try {
		while(1) {
			bool data_available = false;
			while(!data_available) {
				data_available = ((m_socket->poll(1000) == 0) ? true : false);
				if(data_available && m_socket->peek() == 0) {
					printf("IHCServerRequestWorker: Socket seems closed, bailing...\n");
					throw false;
			       	}
			}
			std::string buffer;
			m_socket->recv(buffer,4);
			unsigned int toReceive = 0;
			toReceive += ((unsigned char)buffer[0] << 24);
			toReceive += ((unsigned char)buffer[1] << 16);
			toReceive += ((unsigned char)buffer[2] << 8);
			toReceive += ((unsigned char)buffer[3] << 0);
			m_socket->recv(buffer,toReceive);
			std::istringstream ist(buffer);
			json::Object request;
			json::Reader::Read(request,ist);
//			printf("IHCServerRequestWorker: Request received\n");
//			printf("%s\n",ist.str().c_str());
			json::Object response;
			std::ostringstream ost;
			std::string req = json::String(request["type"]).Value();
			try {
				if(req == "getAll") {
					getAll(response);
				} else if(req == "toggleOutput") {
					toggleOutput(m_token,request,response);
				} else if(req == "activateInput") {
					activateInput(m_token,request,true,response);
				} else if(req == "deactivateInput") {
					activateInput(m_token,request,false,response);
				} else if(req == "getAlarmState") {
					getAlarmState(response);
				} else if(req == "keypadAction") {
					keypadAction(m_token,request,response);
				} else if(req == "getModuleConfiguration") {
					getModuleConfiguration(m_token,request,response);
				} else if(req == "setModuleConfiguration") {
					setModuleConfiguration(m_token,request,response);
				} else if(req == "getUserlevel") {
					getUserlevel(m_token,request,response);
				}
			} catch (bool ex) {
				response["type"] = json::String(req);
				response["result"] = json::Boolean(false);
			}
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
		printf("IHCServerRequestWorker: Exception detected (%s)\n",ex.what());
	} catch (bool ex) {
		// Exception in socket, probably closed
		printf("IHCServerRequestWorker: Exception detected\n");
	}
}
