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
 * This is the main server application for the IHC system.
 * This class owns an instance of the IHCInterface that
 * it uses to serve the clients with requests and events.
 * It owns the two TCPSocketServers that clients connect to
 * depending on what they want.
 *
 * 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCSERVER_H
#define IHCSERVER_H
#include "utils/Observer.h"
#include "utils/Thread.h"
#include "utils/TCPSocketServerCallback.h"
#include "IHCServerDefs.h"
#include <list>

class IHCInterface;
class TCPSocketServer;
class IHCServerWorker;
class IHCServerEventWorker;
class Configuration;
class Reaper;
class IHCIO;

class IHCServer : public Thread, public Observer, public TCPSocketServerCallback {
public:
	IHCServer();
	~IHCServer();
	void thread();

	// The callback that gets connected sockets from the servers
	void socketConnected(TCPSocket* newSocket);
	void deleteServerWorker(IHCServerWorker* worker);

	// We get notified by IHCInputs and IHCOutputs here
	void update(Subject* sub, void* obj);

	// Interface for the socketworkers
	bool getInputState(int moduleNumber, int inputNumber);
	bool getOutputState(int moduleNumber, int outputNumber);
	void setOutputState(int moduleNumber, int outputNumber, bool state);

	void toggleModuleState(enum IHCServerDefs::Type type, int moduleNumber);
	bool getModuleState(enum IHCServerDefs::Type type, int moduleNumber);

	void setIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, std::string description);
	std::string getIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);

	void setIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isProtected);
	bool getIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);

	void saveConfiguration();
private:
	// The interface to the IHC controller
	IHCInterface* m_ihcinterface;

	// Listeners that want to know about changes should register here
	TCPSocketServer* m_eventServer;
	const static int m_eventServerPort = 45201;
	std::list<IHCServerWorker*> m_eventListeners;
	std::list<IHCServerWorker*> m_requestListeners;
	std::list<IHCIO*> m_eventList;
	pthread_cond_t m_eventCond;
	pthread_mutex_t m_eventMutex;
	pthread_mutex_t m_workerMutex;

	// This is for external requests to the server
	TCPSocketServer* m_requestServer;
	const static int m_requestServerPort = 45200;

	// The instance of the configuration
	Configuration* m_configuration;

	const static std::string version;
};

#endif /* IHCSERVER_H */
