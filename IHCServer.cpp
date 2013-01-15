
#include "IHCServer.h"
#include "IHCInterface.h"
#include "utils/TCPSocketServer.h"
#include "IHCServerWorker.h"
#include "IHCServerEventWorker.h"
#include "Configuration.h"
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

IHCServer::IHCServer()
{
	pthread_cond_init(&m_eventCond,NULL);
	pthread_mutex_init(&m_eventMutex,NULL);

	m_configuration = Configuration::getInstance();
	try {
		printf("IHCServer loading configuration.\n");
		m_configuration->load();
	} catch (...) {
		printf("Error in configuration, exitting... Edit config file manually.\n");
		exit(1);
	}

	m_ihcinterface = new IHCInterface(m_configuration->getSerialDevice());
	m_requestServer = new TCPSocketServer(m_requestServerPort,this);
	m_eventServer = new TCPSocketServer(m_eventServerPort,this);
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
		delete io;
	};
}

void IHCServer::update(Subject* sub, void* obj) {
	if(dynamic_cast<IHCOutput*>(sub) != 0) {
		printf("Output %d.%d changed\n",((IHCOutput*)sub)->getModuleNumber(),((IHCOutput*)sub)->getOutputNumber());
		pthread_mutex_lock(&m_eventMutex);
		m_eventList.push_back(new IHCOutput(*(IHCOutput*)sub));
		pthread_cond_signal(&m_eventCond);
		pthread_mutex_unlock(&m_eventMutex);
	} else if(dynamic_cast<IHCInput*>(sub) != 0) {
		printf("Input %d.%d changed\n",((IHCInput*)sub)->getModuleNumber(),((IHCInput*)sub)->getInputNumber());
		pthread_mutex_lock(&m_eventMutex);
		m_eventList.push_back(new IHCInput(*(IHCInput*)sub));
		pthread_cond_signal(&m_eventCond);
		pthread_mutex_unlock(&m_eventMutex);
	}
}

void IHCServer::socketConnected(TCPSocket* newSocket) {
	if(newSocket->getPort() == m_eventServerPort) {
		IHCServerEventWorker* worker = new IHCServerEventWorker(newSocket,this);
		m_eventListeners.push_back(worker);
	} else if(newSocket->getPort() == m_requestServerPort) {
		IHCServerWorker* worker = new IHCServerWorker(newSocket,this);
	}
}

void IHCServer::deleteServerWorker(IHCServerWorker* worker) {
	m_eventListeners.remove(worker);
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
