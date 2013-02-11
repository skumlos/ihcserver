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
#include "Configuration.h"
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

const std::string IHCServer::version = "0.2";

IHCServer::IHCServer()
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

	// Create the interface and tcp socket servers
	m_ihcinterface = new IHCInterface(m_configuration->getSerialDevice());
	m_requestServer = new TCPSocketServer(m_requestServerPort,this);
	m_eventServer = new TCPSocketServer(m_eventServerPort,this);

	// Attach to all IHC I/Os
	for(unsigned int k = 1; k <= 16; k++) {
		for(unsigned int j = 1; j <= 8; j++) {
			m_ihcinterface->getOutput(k,j)->attach(this);
		}
	}
	for(unsigned int k = 1; k <= 8; k++) {
		for(unsigned int j = 1; j <= 16; j++) {
			m_ihcinterface->getInput(k,j)->attach(this);
		}
	}

	// Lets get running
	start();
}

IHCServer::~IHCServer() {
	stop();
	for(unsigned int k = 1; k <= 8; k++) {
		for(unsigned int j = 1; j <= 16; j++) {
			m_ihcinterface->getInput(k,j)->detach(this);
		}
	}
	for(unsigned int k = 1; k <= 16; k++) {
		for(unsigned int j = 1; j <= 8; j++) {
			m_ihcinterface->getOutput(k,j)->detach(this);
		}
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

void IHCServer::thread() {
	m_requestServer->start();
	m_eventServer->start();
	while(m_ihcinterface->isRunning()) {
		pthread_mutex_lock(&m_eventMutex);
		while(m_eventList.empty()) {
			pthread_cond_wait(&m_eventCond,&m_eventMutex);
		}
		IHCIO* io = m_eventList.front();
		m_eventList.pop_front();
		pthread_mutex_unlock(&m_eventMutex);
		std::list<IHCServerWorker*>::const_iterator it;
		pthread_mutex_lock(&m_workerMutex);
		for(it = m_eventListeners.begin(); it != m_eventListeners.end(); it++) {
			if(dynamic_cast<IHCServerEventWorker*>(*it) != 0) {
			        int moduleNumber = io->getModuleNumber();
			        int ioNumber = io->getIONumber();
			        bool state = io->getState();
				if(dynamic_cast<IHCOutput*>(io) != 0) {
					((IHCServerEventWorker*)(*it))->notify(IHCServerDefs::OUTPUT,moduleNumber,ioNumber,state);
				} else if (dynamic_cast<IHCInput*>(io) != 0) {
					((IHCServerEventWorker*)(*it))->notify(IHCServerDefs::INPUT,moduleNumber,ioNumber,state);
				}
			}
		}
		pthread_mutex_unlock(&m_workerMutex);
		delete io;
	};
}

void IHCServer::update(Subject* sub, void* obj) {
	if(dynamic_cast<IHCOutput*>(sub) != 0) {
		pthread_mutex_lock(&m_eventMutex);
		m_eventList.push_back(new IHCOutput(*(IHCOutput*)sub));
		pthread_cond_signal(&m_eventCond);
		pthread_mutex_unlock(&m_eventMutex);
	} else if(dynamic_cast<IHCInput*>(sub) != 0) {
		pthread_mutex_lock(&m_eventMutex);
		m_eventList.push_back(new IHCInput(*(IHCInput*)sub));
		pthread_cond_signal(&m_eventCond);
		pthread_mutex_unlock(&m_eventMutex);
	}
}

void IHCServer::socketConnected(TCPSocket* newSocket) {
	// Event listeners goes onto the list of workers we should notify
	if(newSocket->getPort() == m_eventServerPort) {
		std::string clientID = "";
		newSocket->recv(clientID);
		pthread_mutex_lock(&m_workerMutex);
		std::list<IHCServerWorker*>::iterator it = m_eventListeners.begin();
		for(; it != m_eventListeners.end(); it++) {
			if(dynamic_cast<IHCServerEventWorker*>(*it) != 0) {
				if(clientID == ((IHCServerEventWorker*)(*it))->getClientID()) {
					((IHCServerEventWorker*)(*it))->setSocket(newSocket);
					break;
				}
			}
		}
		if(it == m_eventListeners.end()) {
			IHCServerEventWorker* worker = new IHCServerEventWorker(clientID,newSocket,this);
			m_eventListeners.push_back(worker);
		}
		pthread_mutex_unlock(&m_workerMutex);
	} else if(newSocket->getPort() == m_requestServerPort) {
		IHCServerWorker* worker = new IHCServerWorker(newSocket,this);
	}
}

void IHCServer::deleteServerWorker(IHCServerWorker* worker) {
	pthread_mutex_lock(&m_workerMutex);
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

void IHCServer::saveConfiguration() {
	m_configuration->save();
}
