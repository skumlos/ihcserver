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

IHCServerRequestWorker::IHCServerRequestWorker(std::string clientID, TCPSocket* socket, IHCServer* server) :
	IHCServerWorker(clientID,socket,server),
	m_token(0)
{
	start();
}

IHCServerRequestWorker::~IHCServerRequestWorker() {}

void IHCServerRequestWorker::thread() {
	signal(SIGPIPE,SIG_IGN);
	try {
		while(1) {
			bool data_available = false;
			while(!data_available) {
				data_available = ((m_socket->poll(1000) == 0) ? true : false);
				if(data_available && m_socket->peek() == 0) {
					// Socket is closed, lets see if another will get in...
					struct timespec t;
					clock_gettime(CLOCK_REALTIME,&t);
					t.tv_sec += 600;
					int ret = pthread_cond_timedwait(&m_socketCond,&m_socketMutex,&t);
					pthread_mutex_unlock(&m_socketMutex);
					if(ret == ETIMEDOUT) {
						// nope... bail out!
						throw false;
					}
					data_available = false;
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
//			printf("Request: %s\n",buffer.c_str());
			std::istringstream ist(buffer);
			json::Object req;
			json::Reader::Read(req,ist);

			json::Object resp;
			std::ostringstream ost;
			// Decode request and form response
			try {
				json::String type = req["type"];
				if(type.Value() == "login") {
					m_socket->send(std::string("ACK"));
					std::string input = json::String(req["input"]).Value();
					m_socket->send(std::string("ACK"));
					try {
						Userlevel::login(m_token,input);
					} catch(...) {
						printf("Exception when logging in\n");
					}
				} else if(type.Value() == "arm") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::SUPERUSER) {
						printf("%s trying to arm without proper level\n",m_clientID.c_str());
						throw false;
					}
					m_socket->send(std::string("ACK"));
					std::string input = json::String(req["input"]).Value();
					printf("%s arms with %s\n",m_clientID.c_str(),input.c_str());
				} else if(type.Value() == "disarm") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::SUPERUSER) {
						printf("%s trying to disarm without proper level\n",m_clientID.c_str());
						throw false;
					}
					m_socket->send(std::string("ACK"));
					std::string input = json::String(req["input"]).Value();
					printf("%s disarms with %s\n",m_clientID.c_str(),input.c_str());
				} else if(type.Value() == "checkUserlevel") {
					printf("checkUserlevel\n%s\n",buffer.c_str());
					std::string level = json::String(req["data"]).Value();
					m_socket->send(std::string("ACK"));
					bool ret = false;
					if(level == "admin" && Userlevel::getUserlevel(m_token) == Userlevel::ADMIN) {
						ret = true;
					} else if (level == "superuser" && Userlevel::getUserlevel(m_token) == Userlevel::SUPERUSER) {
						ret = true;
					}
					resp["type"] = type;
					resp["level"] = json::String(level);
					resp["result"] = json::Boolean(ret);
				} else if(type.Value() == "getOutput") {
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					int outputNumber = json::Number(req["ioNumber"]).Value();
					m_socket->send(std::string("ACK"));
					bool state = m_server->getOutputState(moduleNumber,outputNumber);
					resp["type"] = json::String("outputState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["outputNumber"] = json::Number(outputNumber);
					resp["state"] = json::Boolean(state);
				} else if(type.Value() == "toggleOutput") {
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					int outputNumber = json::Number(req["ioNumber"]).Value();
					m_socket->send(std::string("ACK"));
					if(m_server->getIOProtected(IHCServerDefs::OUTPUTMODULE,moduleNumber,outputNumber) &&
					(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN &&
					Userlevel::getUserlevel(m_token) != Userlevel::SUPERUSER)) {
						throw false;
					}
					bool state = m_server->getOutputState(moduleNumber,outputNumber);
					m_server->setOutputState(moduleNumber,outputNumber,!state);
					resp["type"] = json::String("outputState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["outputNumber"] = json::Number(outputNumber);
					resp["state"] = json::Boolean(state);
				} else if(type.Value() == "getModuleConfiguration") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN) {
						printf("%s trying to getModuleConfiguration without proper level\n",m_clientID.c_str());
						throw false;
					}
					std::string moduleType = json::String(req["moduleType"]).Value();
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					m_socket->send(std::string("ACK"));
					resp["type"] = json::String("moduleConfiguration");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["moduleType"] = json::String(moduleType);
					enum IHCServerDefs::Type type;
					int ios = 0;
					if(moduleType == "input") {
						type = IHCServerDefs::INPUTMODULE;
						ios = 16;
					} else if(moduleType == "output") {
						type = IHCServerDefs::OUTPUTMODULE;
						ios = 8;
					} else {
						throw false;
					}
					resp["state"] = json::Boolean(m_server->getModuleState(type,moduleNumber));
					json::Array ioDescriptions;
					for(int j = 1; j <= ios; j++) {
						json::Object ioDescription;
						if(m_server->getIODescription(type,moduleNumber,j) != "") {
							ioDescription["ioNumber"] = json::Number(j);
							ioDescription["ioDescription"] = json::String(m_server->getIODescription(type,moduleNumber,j));
//							printf("Description %s found\n",m_server->getIODescription(type,moduleNumber,j).c_str());
							ioDescriptions.Insert(ioDescription);
						}
					}
					resp["ioDescriptions"] = ioDescriptions;
					json::Array ioProtectedStates;
					for(int j = 1; j <= ios; j++) {
						json::Object ioProtected;
						if(m_server->getIOProtected(type,moduleNumber,j)) {
							ioProtected["ioNumber"] = json::Number(j);
							ioProtected["ioProtected"] = json::Boolean(true);
							ioProtectedStates.Insert(ioProtected);
						}
					}
					resp["ioProtectedStates"] = ioProtectedStates;
				} else if(type.Value() == "setModuleConfiguration") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN) {
						printf("%s trying to setModuleConfiguration without proper level\n",m_clientID.c_str());
						throw false;
					}
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					std::string moduleType = json::String(req["moduleType"]).Value();
					enum IHCServerDefs::Type type;
					int ios = 0;
					if(moduleType == "input") {
						type = IHCServerDefs::INPUTMODULE;
					} else if(moduleType == "output") {
						type = IHCServerDefs::OUTPUTMODULE;
					} else {
						throw false;
					}

					json::Array::const_iterator it;
					json::Array ioDescriptions = json::Array(req["ioDescriptions"]);
					for(it = ioDescriptions.Begin(); it != ioDescriptions.End(); it++) {
						json::Object ioDescriptionObj = json::Object(*it);
						int ioNumber = json::Number(ioDescriptionObj["ioNumber"]).Value();
						std::string ioDescription = json::String(ioDescriptionObj["ioDescription"]).Value();
						if(ioDescription != "") {
							m_server->setIODescription(type,moduleNumber,ioNumber,ioDescription);
//							printf("IODescription for %s %d.%d: %s\n",moduleType.c_str(),moduleNumber,ioNumber,ioDescription.c_str());
						}
					}
					json::Array ioProtectedStates = json::Array(req["ioProtectedStates"]);
					for(it = ioProtectedStates.Begin(); it != ioProtectedStates.End(); it++) {
						json::Object ioProtectedObj = json::Object(*it);
						int ioNumber = json::Number(ioProtectedObj["ioNumber"]).Value();
						bool ioProtected = json::Boolean(ioProtectedObj["ioProtected"]).Value();
						m_server->setIOProtected(type,moduleNumber,ioNumber,ioProtected);
					}
					m_socket->send(std::string("ACK"));
					m_server->saveConfiguration();
				} else if(type.Value() == "getOutputModuleState") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN) {
						printf("%s trying to getOutputModuleState without proper level\n",m_clientID.c_str());
						throw false;
					}
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					m_socket->send(std::string("ACK"));
					resp["type"] = json::String("outputModuleState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["state"] = json::Boolean(m_server->getModuleState(IHCServerDefs::OUTPUTMODULE,moduleNumber));
				} else if(type.Value() == "getInputModuleState") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN) {
						printf("%s trying to getInputModuleState without proper level\n",m_clientID.c_str());
						throw false;
					}
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					m_socket->send(std::string("ACK"));
					resp["type"] = json::String("inputModuleState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["state"] = json::Boolean(m_server->getModuleState(IHCServerDefs::INPUTMODULE,moduleNumber));
				} else if(type.Value() == "toggleOutputModule") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN) {
						printf("%s trying to toggleOutputModule without proper level\n",m_clientID.c_str());
						throw false;
					}
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					m_socket->send(std::string("ACK"));
					m_server->toggleModuleState(IHCServerDefs::OUTPUTMODULE,moduleNumber);
					resp["type"] = json::String("outputModuleState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["state"] = json::Boolean(m_server->getModuleState(IHCServerDefs::OUTPUTMODULE,moduleNumber));
				} else if(type.Value() == "toggleInputModule") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN) {
						printf("%s trying to toggleInputModule without proper level\n",m_clientID.c_str());
						throw false;
					}
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					m_socket->send(std::string("ACK"));
					m_server->toggleModuleState(IHCServerDefs::INPUTMODULE,moduleNumber);
					resp["type"] = json::String("inputModuleState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["state"] = json::Boolean(m_server->getModuleState(IHCServerDefs::INPUTMODULE,moduleNumber));
				} else if(type.Value() == "getAll") {
					m_socket->send(std::string("ACK"));
					getAll(resp);
				} else if(type.Value() == "saveConfiguration") {
					if(Userlevel::getUserlevel(m_token) != Userlevel::ADMIN) {
						printf("%s trying to saveConfiguration without proper level\n",m_clientID.c_str());
						throw false;
					}
					m_socket->send(std::string("ACK"));
					m_server->saveConfiguration();
				} else {
					printf("Unknown request\n");
					printf("Request: %s\n",buffer.c_str());
					throw false;
				}
				// Ship that response via the socket
				if(!resp.Empty()) {
					json::Writer::Write(resp,ost);
					int stringlength = ost.str().size();
					unsigned char* header = new unsigned char[4];
					header[0] = (unsigned char) (stringlength >> 24);
					header[1] = (unsigned char) (stringlength >> 16);
					header[2] = (unsigned char) (stringlength >> 8);
					header[3] = (unsigned char) (stringlength >> 0);
					m_socket->send(header,4);
					m_socket->send(ost.str());
				}
			} catch (...) {
				printf("IHCServerRequestWorker: Exception in json parsing\n");
				printf("Request: %s\n",buffer.c_str());
				m_socket->send(std::string("NAK"));
			}
		}
	} catch (...) {
	// Exception in socket, probably closed
	}
}

void IHCServerRequestWorker::getAll(json::Object& resp) {
	resp["type"] = json::String("allModules");
	json::Object modules;
	json::Array inputModules;
	for(unsigned int j = 1; j <= 8; j++) {
		json::Object module;
		module["moduleNumber"] = json::Number(j);
		bool state = m_server->getModuleState(IHCServerDefs::INPUTMODULE,j);
		module["state"] = json::Boolean(state);
		if(state) {
			json::Array inputStates;
			for(unsigned int k = 1; k <= 16; k++) {
				json::Object inputState;
				inputState["inputNumber"] = json::Number(k);
				inputState["inputState"] = json::Boolean(m_server->getInputState(j,k));
				inputState["description"] = json::String(m_server->getIODescription(IHCServerDefs::INPUTMODULE,j,k));
				inputState["protected"] = json::Boolean(m_server->getIOProtected(IHCServerDefs::INPUTMODULE,j,k));
				inputStates.Insert(inputState);
			}
			module["inputStates"] = inputStates;
		}
		inputModules.Insert(module);
	}
	modules["inputModules"] = inputModules;
	json::Array outputModules;
	for(unsigned int j = 1; j <= 16; j++) {
		json::Object module;
		module["moduleNumber"] = json::Number(j);
		bool state = m_server->getModuleState(IHCServerDefs::OUTPUTMODULE,j);
		module["state"] = json::Boolean(state);
		if(state) {
			json::Array outputStates;
			for(unsigned int k = 1; k <= 8; k++) {
				json::Object outputState;
				outputState["outputNumber"] = json::Number(k);
				outputState["outputState"] = json::Boolean(m_server->getOutputState(j,k));
				outputState["description"] = json::String(m_server->getIODescription(IHCServerDefs::OUTPUTMODULE,j,k));
				outputState["protected"] = json::Boolean(m_server->getIOProtected(IHCServerDefs::OUTPUTMODULE,j,k));
				outputStates.Insert(outputState);
			}
			module["outputStates"] = outputStates;
		}
		outputModules.Insert(module);
	}
	modules["outputModules"] = outputModules;
	resp["modules"] = modules;
}
