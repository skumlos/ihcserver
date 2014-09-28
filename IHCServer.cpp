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

#include "IHCServer.h"
#include "IHCInterface.h"
#include "utils/TCPSocketServer.h"
#include "IHCServerWorker.h"
#include "IHCServerEventWorker.h"
#include "IHCServerRequestWorker.h"
#include "IHCEvent.h"
#include "Configuration.h"
#include "Userlevel.h"
#include "IHCHTTPServer.h"
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

const std::string IHCServer::version = "0.3";

IHCServer* IHCServer::m_instance = NULL;
pthread_mutex_t IHCServer::m_instanceMutex = PTHREAD_MUTEX_INITIALIZER;

IHCServer* IHCServer::getInstance() {
	pthread_mutex_lock(&m_instanceMutex);
	if(m_instance == NULL) {
		m_instance = new IHCServer();
	}
	pthread_mutex_unlock(&m_instanceMutex);
	return m_instance;
};

IHCServer::IHCServer() :
	m_alarmState(false)
{
	printf("\nIHCServer v%s\nby Martin Hejnfelt\n\n",version.c_str());
	pthread_cond_init(&m_eventCond,NULL);
	pthread_mutex_init(&m_eventMutex,NULL);
	pthread_mutex_init(&m_workerMutex,NULL);

	// Initialize and load the configuration
	m_configuration = Configuration::getInstance();
	try {
		printf("IHCServer loading configuration.\n");
		m_configuration->load();
	} catch (...) {
		printf("Error in configuration, exitting... Edit config file manually.\n");
		exit(1);
	}

	// Initialize userlevel subsystem
	Userlevel::init();

	// Create the interface and tcp socket servers
	m_ihcinterface = new IHCInterface(m_configuration->getSerialDevice());
	m_requestServer = new TCPSocketServer(m_requestServerPort,this);
	m_eventServer = new TCPSocketServer(m_eventServerPort,this);
	m_httpServer = IHCHTTPServer::getInstance();
	attach(m_httpServer);

	// Attach to all IHC I/Os
	std::list<IHCInput*> inputs = m_ihcinterface->getAllInputs();
	std::list<IHCInput*>::iterator input = inputs.begin();
	for(; input != inputs.end(); ++input) {
		(*input)->attach(this);
	}

	std::list<IHCOutput*> outputs = m_ihcinterface->getAllOutputs();
	std::list<IHCOutput*>::iterator output = outputs.begin();
	for(; output != outputs.end(); ++output) {
		(*output)->attach(this);
	}

	// Lets get running
	start();
}

IHCServer::~IHCServer() {
	stop();
	std::list<IHCInput*> inputs = m_ihcinterface->getAllInputs();
	std::list<IHCInput*>::iterator input = inputs.begin();
	for(; input != inputs.end(); ++input) {
		(*input)->detach(this);
	}

	std::list<IHCOutput*> outputs = m_ihcinterface->getAllOutputs();
	std::list<IHCOutput*>::iterator output = outputs.begin();
	for(; output != outputs.end(); ++output) {
		(*output)->detach(this);
	}
	m_eventServer->stop();
	m_requestServer->stop();
	delete m_eventServer;
	delete m_requestServer;
	m_ihcinterface->stop();
	delete m_ihcinterface;
	pthread_mutex_destroy(&m_workerMutex);
	pthread_mutex_destroy(&m_eventMutex);
	pthread_cond_destroy(&m_eventCond);
}

bool IHCServer::getInputState(int moduleNumber, int inputNumber) {
	bool ret = false;
	IHCInput* inp = m_ihcinterface->getInput(moduleNumber,inputNumber);
	if(inp != NULL) {
		ret = inp->getState();
	}
	return ret;
}

bool IHCServer::getOutputState(int moduleNumber, int outputNumber) {
	bool ret = false;
	IHCOutput* out = m_ihcinterface->getOutput(moduleNumber,outputNumber);
	if(out != NULL) {
		ret = out->getState();
	}
	return ret;
}

void IHCServer::setOutputState(int moduleNumber, int outputNumber, bool state) {
	IHCOutput* output = m_ihcinterface->getOutput(moduleNumber,outputNumber);
	if(output != NULL) {
		m_ihcinterface->changeOutput(output,state);
	}
	return;
}

void IHCServer::activateInput(int moduleNumber, int inputNumber, bool shouldActivate) {
	IHCInput* input = m_ihcinterface->getInput(moduleNumber,inputNumber);
	if(input != NULL) {
		m_ihcinterface->changeInput(input,shouldActivate);
	}
	return;
}

void IHCServer::thread() {
	m_requestServer->start();
	m_eventServer->start();
	while(m_ihcinterface->isRunning()) {
		pthread_mutex_lock(&m_eventMutex);
		while(m_eventList.empty()) {
			pthread_cond_wait(&m_eventCond,&m_eventMutex);
		}
		IHCEvent* event = m_eventList.front();
		m_eventList.pop_front();
		pthread_mutex_unlock(&m_eventMutex);
		std::list<IHCServerWorker*>::const_iterator it;
		pthread_mutex_lock(&m_workerMutex);
		for(it = m_eventListeners.begin(); it != m_eventListeners.end(); it++) {
			if(event->m_event == IHCServerDefs::OUTPUT_CHANGED ||
				event->m_event == IHCServerDefs::INPUT_CHANGED) {
				if(event->m_io != NULL) {
			        	int moduleNumber = event->m_io->getModuleNumber();
			        	int ioNumber = event->m_io->getIONumber();
			        	bool state = event->m_io->getState();
					if(dynamic_cast<IHCOutput*>(event->m_io) != 0) {
						((IHCServerEventWorker*)(*it))->notify(IHCServerDefs::OUTPUT,moduleNumber,ioNumber,state);
					} else if (dynamic_cast<IHCInput*>(event->m_io) != 0) {
						((IHCServerEventWorker*)(*it))->notify(IHCServerDefs::INPUT,moduleNumber,ioNumber,state);
					}
				}
			} else {
				((IHCServerEventWorker*)(*it))->notify(event->m_event);
			}
		}
		notify((void*)event);
		pthread_mutex_unlock(&m_workerMutex);
		if(event->m_io != NULL) {
			delete event->m_io;
		}
		delete event;
	};
}

void IHCServer::update(Subject* sub, void* obj) {
	if(dynamic_cast<IHCOutput*>(sub) != 0) {
		IHCEvent* event = new IHCEvent();
		event->m_event = IHCServerDefs::OUTPUT_CHANGED;
		event->m_io = new IHCOutput(*(IHCOutput*)sub);
		if(m_configuration->getIOAlarm(IHCServerDefs::OUTPUTMODULE,event->m_io->getModuleNumber(),event->m_io->getIONumber())) {
			setAlarmState(event->m_io->getState());
		}
		pthread_mutex_lock(&m_eventMutex);
		m_eventList.push_back(event);
		pthread_cond_signal(&m_eventCond);
		pthread_mutex_unlock(&m_eventMutex);
	} else if(dynamic_cast<IHCInput*>(sub) != 0) {
		IHCEvent* event = new IHCEvent();
		event->m_event = IHCServerDefs::INPUT_CHANGED;
		event->m_io = new IHCInput(*(IHCInput*)sub);
		if(m_configuration->getIOAlarm(IHCServerDefs::INPUTMODULE,event->m_io->getModuleNumber(),event->m_io->getIONumber())) {
			setAlarmState(event->m_io->getState());
		}
		pthread_mutex_lock(&m_eventMutex);
		m_eventList.push_back(event);
		pthread_cond_signal(&m_eventCond);
		pthread_mutex_unlock(&m_eventMutex);
	}
}

void IHCServer::socketConnected(TCPSocket* newSocket) {
	std::string clientID = "";
/*	if(newSocket->poll(2000) == 0) {
		// No client id available...
		printf("No Session ID received, bailing out\n");
		delete newSocket;
		return;
	}*/
	newSocket->recv(clientID);
	pthread_mutex_lock(&m_workerMutex);
	if(newSocket->getPort() == m_eventServerPort) {
		std::list<IHCServerWorker*>::iterator it = m_eventListeners.begin();
		for(; it != m_eventListeners.end(); it++) {
			if(clientID == (*it)->getClientID()) {
//				printf("Found running event session (%s)\n",clientID.c_str());
				(*it)->setSocket(newSocket);
				break;
			}
		}
		if(it == m_eventListeners.end()) {
//			printf("No running session found (%s), creating new EventWorker\n",clientID.c_str());
			IHCServerEventWorker* worker = new IHCServerEventWorker(clientID,newSocket,this);
			// Event listeners goes onto the list of workers we should notify
			m_eventListeners.push_back(worker);
		}
	} else if(newSocket->getPort() == m_requestServerPort) {
		std::list<IHCServerWorker*>::iterator it = m_requestListeners.begin();
		for(; it != m_requestListeners.end(); it++) {
			if(clientID == (*it)->getClientID()) {
//				printf("Found running request session (%s)\n",clientID.c_str());
				(*it)->setSocket(newSocket);
				break;
			}
		}
		if(it == m_requestListeners.end()) {
//			printf("No running session found (%s), creating new RequestWorker\n",clientID.c_str());
			IHCServerWorker* worker = new IHCServerRequestWorker(clientID,newSocket,this);
			m_requestListeners.push_back(worker);
		}
	}
	pthread_mutex_unlock(&m_workerMutex);
}

void IHCServer::deleteServerWorker(IHCServerWorker* worker) {
	pthread_mutex_lock(&m_workerMutex);
	m_requestListeners.remove(worker);
	m_eventListeners.remove(worker); // If on the list, remove
	pthread_mutex_unlock(&m_workerMutex);
}

void IHCServer::toggleModuleState(enum IHCServerDefs::Type type, int moduleNumber) {
	bool currentState = m_configuration->getModuleState(type,moduleNumber);
	m_configuration->setModuleState(type,moduleNumber,!currentState);
	return;
}

bool IHCServer::getModuleState(enum IHCServerDefs::Type type, int moduleNumber) {
	return m_configuration->getModuleState(type,moduleNumber);
}

void IHCServer::setIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, std::string description) {
	m_configuration->setIODescription(type,moduleNumber,ioNumber,description);
	return;
}

std::string IHCServer::getIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber) {
	return m_configuration->getIODescription(type,moduleNumber,ioNumber);
}

void IHCServer::setIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isProtected) {
	m_configuration->setIOProtected(type,moduleNumber,ioNumber,isProtected);
}

bool IHCServer::getIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber) {
	return m_configuration->getIOProtected(type,moduleNumber,ioNumber);
}

void IHCServer::setIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isAlarm) {
	m_configuration->setIOAlarm(type,moduleNumber,ioNumber,isAlarm);
}

bool IHCServer::getIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber) {
	return m_configuration->getIOAlarm(type,moduleNumber,ioNumber);
}

bool IHCServer::getAlarmState() {
	bool ret = false;
	for(unsigned int j = 1; j<=8; j++) {
		for(unsigned int k = 1; k<=16; k++) {
			if(m_configuration->getIOAlarm(IHCServerDefs::INPUTMODULE,j,k) &&
				m_ihcinterface->getInput(j,k)->getState()) {
				ret = true;
				break;
			}
		}
	}
	if(!ret) {
		for(unsigned int j = 1; j<=16; j++) {
			for(unsigned int k=1; k<=8; k++) {
				if(m_configuration->getIOAlarm(IHCServerDefs::OUTPUTMODULE,j,k) &&
					m_ihcinterface->getOutput(j,k)->getState()) {
					ret = true;
					break;
				}
			}
		}
	}
	return ret;
}

void IHCServer::setAlarmState(bool alarmState) {
	bool oldState = m_alarmState;
	m_alarmState = alarmState;
	printf("New AlarmState %s, old alarmstate %s\n",(m_alarmState ? "TRUE" : "FALSE"), (oldState ? "TRUE" : "FALSE")); 
	if(m_alarmState != oldState) {
		IHCEvent* event = new IHCEvent();
		event->m_event = m_alarmState ? IHCServerDefs::ALARM_ARMED : IHCServerDefs::ALARM_DISARMED;
/*		for(unsigned int j = 1; j<=8; j++) {
			for(unsigned int k = 1; k<=16; k++) {
				if(m_configuration->getIOAlarm(IHCServerDefs::INPUTMODULE,j,k)) {
					IHCInput* input = m_ihcinterface->getInput(j,k);
				}
			}
		}*/
		for(unsigned int j = 1; j<=16; j++) {
			for(unsigned int k = 1; k<=8; k++) {
				if(m_configuration->getIOAlarm(IHCServerDefs::OUTPUTMODULE,j,k)) {
					IHCOutput* output = m_ihcinterface->getOutput(j,k);
					m_ihcinterface->changeOutput(output,m_alarmState);
				}
			}
		}
		pthread_mutex_lock(&m_eventMutex);
		m_eventList.push_back(event);
		pthread_cond_signal(&m_eventCond);
		pthread_mutex_unlock(&m_eventMutex);
	}
}

void IHCServer::saveConfiguration() {
	m_configuration->save();
}
