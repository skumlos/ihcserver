#include "IHCServer.h"
#include "IHCInterface.h"
#include "utils/TCPSocketServer.h"
#include "IHCServerWorker.h"
#include <unistd.h>
#include <cstdio>

IHCServer::IHCServer()
{
	m_ihcinterface = new IHCInterface("/dev/ttyS1");
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

void IHCServer::thread() {
	m_requestServer->start();
	m_eventServer->start();
	while(m_ihcinterface->isRunning()) { sleep(100); };
}

void IHCServer::update(Subject* sub, void* obj) {
	if(dynamic_cast<IHCOutput*>(sub) != 0) {
		printf("Output %d.%d changed\n",((IHCOutput*)sub)->getModuleNumber(),((IHCOutput*)sub)->getOutputNumber());
	} else if(dynamic_cast<IHCInput*>(sub) != 0) {
		printf("Input %d.%d changed\n",((IHCInput*)sub)->getModuleNumber(),((IHCInput*)sub)->getInputNumber());
	}
}

void IHCServer::socketConnected(TCPSocket* newSocket) {
	IHCServerWorker* worker = new IHCServerWorker(newSocket,this);
}

