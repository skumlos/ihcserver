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
 * This is the main interface to the IHC controller
 * It works by waiting for the DATA_READY/GIVE command (0x30)
 * addressed to the PC (ID_PC/0x1C). When such a packet is received
 * it checks if any requests are in, if not it either updates inputs,
 * or outputs, depending on what was updated upon last DATA_READY
 * "event".
 *
 * December 2012, January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCINTERFACE_H
#define IHCINTERFACE_H
#include <string>
#include <vector>
#include <list>
#include <map>
#include <pthread.h>
#include "utils/Thread.h"
#include "IHCDefs.h"
#include "IHCRS485Packet.h"
#include "IHCOutput.h"
#include "IHCInput.h"

class UART;

class IHCInterface : public Thread {
public:
	IHCInterface(std::string rs485port);
	~IHCInterface();
	void thread();
	IHCInput* getInput(int moduleNumber, int inputNumber);
	IHCOutput* getOutput(int moduleNumber, int outputNumber);
	void changeOutput(IHCOutput* output, bool newState);
private:
	IHCRS485Packet getPacket(UART& uart, int ID = IHCDefs::ID_PC, bool useTimeout = true) throw (bool);
	void updateInputStates(const std::vector<unsigned char>& newStates);
	void updateOutputStates(const std::vector<unsigned char>& newStates);

	UART* m_port;
	std::map<int,std::vector<IHCOutput*> > m_outputs;
	std::map<int,std::vector<IHCInput*> > m_inputs;
	pthread_mutex_t m_packetQueueMutex;
	std::list<IHCRS485Packet> m_packetQueue;
};

#endif /* IHCINTERFACE_H */
