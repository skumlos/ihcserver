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


#include "IHCInterface.h"
#include "utils/Uart.h"
#include <cstdlib>
#include <ctime>
#include "Configuration.h"

#define DEBUG (0)

static unsigned int packets = 0;

void printTimeStamp(time_t t = 0) {
        time_t now = (t == 0 ? time(NULL) : t);
        struct tm* timeinfo;
        timeinfo = localtime(&now);
        printf("%.2i:%.2i:%.2i ",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
        return;
};

IHCInterface::IHCInterface(std::string rs485port)
{
	try {
		m_port = new UART(rs485port,Configuration::getInstance()->useHWFlowControl());
//		m_port = new UART(rs485port,true);
		m_port->setSpeed(B19200);
	} catch (std::exception& ex) {
		printf("IHCInterface: Could not init UART (%s)\n",ex.what());
		throw false;
	}

	pthread_mutex_init(&m_packetQueueMutex,NULL);

	for(unsigned int j = 0; j < 16; ++j) {
		std::vector<IHCOutput*> v;
		for(unsigned int k = 0; k < 8; ++k) {
			IHCOutput* output = new IHCOutput(j+1,k+1);
			v.push_back(output);
		}
		m_outputs[j] = v;
	}
	for(unsigned int j = 0; j < 8; ++j) {
		std::vector<IHCInput*> v;
		for(unsigned int k = 0; k < 16; ++k) {
			IHCInput* input = new IHCInput(j+1,k+1);
			v.push_back(input);
		}
		m_inputs[j] = v;
	}
	start();
}

IHCInterface::~IHCInterface() {
	pthread_mutex_destroy(&m_packetQueueMutex);
}

void IHCInterface::changeOutput(IHCOutput* output, bool newState) {
	std::vector<unsigned char> data;
	data.push_back((unsigned char) (((output->getModuleNumber()-1)*10)+output->getOutputNumber()));
	data.push_back(newState ? (unsigned char)1 : (unsigned char)0);
	IHCRS485Packet packet(IHCDefs::ID_IHC,IHCDefs::SET_OUTPUT,&data);
	pthread_mutex_lock(&m_packetQueueMutex);
	m_packetQueue.push_back(packet);
	pthread_mutex_unlock(&m_packetQueueMutex);
}

void IHCInterface::changeInput(IHCInput* input, bool shouldActivate) {
	std::vector<unsigned char> data;
	data.push_back((unsigned char) (((input->getModuleNumber()-1)*10)+input->getInputNumber()));
	data.push_back(shouldActivate ? (unsigned char)1 : (unsigned char)0);
	IHCRS485Packet packet(IHCDefs::ID_IHC,IHCDefs::ACT_INPUT,&data);
	pthread_mutex_lock(&m_packetQueueMutex);
	m_packetQueue.push_back(packet);
	pthread_mutex_unlock(&m_packetQueueMutex);
}

void IHCInterface::updateInputStates(const std::vector<unsigned char>& newStates) {
	for(unsigned int k = 0; k < 8; ++k) {
		unsigned int ioModuleState = newStates[(k*2)] + (newStates[(k*2)+1] << 8);
		for(unsigned int j = 0; j < 16; ++j) {
			bool state = (((ioModuleState & (1 << j)) > 0) ? true : false);
			if(m_inputs[k][j]->getState() != state) {
				if(state) {
					printTimeStamp();
					printf("Input %d.%d is ON\n",k+1,j+1);
				} else {
					printTimeStamp();
					printf("Input %d.%d is OFF\n",k+1,j+1);
				}
			}
			m_inputs[k][j]->setState(state);
		}
	}
}

void IHCInterface::updateOutputStates(const std::vector<unsigned char>& newStates) {
	for(unsigned int k = 0; k < newStates.size(); ++k) {
		unsigned char ioModuleState = newStates[k];
		for(unsigned int j = 0; j < 8; ++j) {
			bool state = (((ioModuleState & (1 << j)) > 0) ? true : false);
			if(m_outputs[k][j]->getState() != state) {
				if(state) {
					printTimeStamp();
					printf("Output %d.%d is ON\n",k+1,j+1);
				} else {
					printTimeStamp();
					printf("Output %d.%d is OFF\n",k+1,j+1);
				}
			}
			m_outputs[k][j]->setState(state);
		}
	}
}

IHCRS485Packet IHCInterface::getPacket(UART& uart, int ID, bool useTimeout) throw (bool) {
	unsigned char c = 0;
	std::vector<unsigned char> packet;
	uart.poll();
	time_t timeout_s = time(NULL) + 5;
	int out_of_frame_bytes = 0;
	while(1) {
		out_of_frame_bytes = 0;
		c = uart.readByte();
		while(c != IHCDefs::STX) { // Syncing with STX
//			printf("%.2X \n",c);
			out_of_frame_bytes++;
			c = uart.readByte();
		}
/*		if(out_of_frame_bytes > 0) {
			printf("\n");
		}*/
		packet.push_back(c);
		while(c != IHCDefs::ETB) { // Reading frame
			c = uart.readByte();
			packet.push_back(c);
		}
		c = uart.readByte(); // Reading CRC
		packet.push_back(c);
		IHCRS485Packet p(packet);
		if(p.isComplete() && p.getID() == ID) {
			if(DEBUG) p.print();
//			++packets;
//			printf("IHCInterface received: %d packets\t\t\t\r",packets);
//			fflush(stdout);
			return p;
		}
		packet.clear();
		if(useTimeout && time(NULL) >= timeout_s) throw false;
	}
};

void IHCInterface::thread() {
	std::vector<unsigned char> outputBlocks(16,0x0);
	std::vector<unsigned char> inputBlocks(16,0x0);

	bool sendPackets = false;

	int id = IHCDefs::ID_PC;

	IHCRS485Packet getInputPacket(IHCDefs::ID_IHC, IHCDefs::GET_INPUTS);
	IHCRS485Packet getOutputPacket(IHCDefs::ID_IHC, IHCDefs::GET_OUTPUTS);

	int lastUpdated = IHCDefs::INP_STATE;
	std::vector<unsigned char> io;

	while(1) {
		try {

			IHCRS485Packet packet = getPacket(*m_port,id,false);
			switch(packet.getDataType()) {
				case IHCDefs::ACK:
					if(DEBUG) printf("IHCInterface: Packet was ACK'ed\n");
				break;
				case IHCDefs::DATA_READY:
					pthread_mutex_lock(&m_packetQueueMutex);
					if(!m_packetQueue.empty() && sendPackets) {
						IHCRS485Packet toSend = m_packetQueue.front();
						if(DEBUG) {
							printf("IHCInterface: Sending packet\n");
							toSend.print();
						}
						m_port->write(toSend.getPacket());
						m_packetQueue.pop_front();
						sendPackets = true;
					} else {
						sendPackets = false;
					}
					pthread_mutex_unlock(&m_packetQueueMutex);
					if(!sendPackets) {
						if(lastUpdated == IHCDefs::INP_STATE) {
							m_port->write(getOutputPacket.getPacket());
						} else {
							m_port->write(getInputPacket.getPacket());
						}
					}
				break;
				case IHCDefs::INP_STATE:
					io = packet.getData();
					updateInputStates(io);
					lastUpdated = IHCDefs::INP_STATE;
					sendPackets = true;
				break;
				case IHCDefs::OUTP_STATE:
					io = packet.getData();
					updateOutputStates(io);
					lastUpdated = IHCDefs::OUTP_STATE;
					sendPackets = true;
				break;
			}
		} catch (std::exception& ex) {
			printf("IHCInterface: Caught exception in communication thread (%s)\n",ex.what());
			exit(1);
		}
	}
}

IHCInput* IHCInterface::getInput(int moduleNumber, int inputNumber) {
	if(moduleNumber < 1 || moduleNumber > 8 || inputNumber > 16 || inputNumber < 1) {
		return NULL;
	}
	return m_inputs[moduleNumber-1][inputNumber-1];
}

IHCOutput* IHCInterface::getOutput(int moduleNumber, int outputNumber) {
	if(moduleNumber < 1 || moduleNumber > 16 || outputNumber > 8 || outputNumber < 1) {
		return NULL;
	}
	return m_outputs[moduleNumber-1][outputNumber-1];
}

std::list<IHCInput*> IHCInterface::getAllInputs() {
	std::list<IHCInput*> ret;
	std::map<int,std::vector<IHCInput*> >::iterator it = m_inputs.begin();
	for(; it != m_inputs.end(); ++it) {
		std::vector<IHCInput*> inputs = it->second;
		std::vector<IHCInput*>::const_iterator input = inputs.begin();
		for(; input != inputs.end(); ++input) {
			ret.push_back((*input));
		}
	}
	return ret;
}

std::list<IHCOutput*> IHCInterface::getAllOutputs() {
	std::list<IHCOutput*> ret;
	std::map<int,std::vector<IHCOutput*> >::iterator it = m_outputs.begin();
	for(; it != m_outputs.end(); ++it) {
		std::vector<IHCOutput*> outputs = it->second;
		std::vector<IHCOutput*>::const_iterator output = outputs.begin();
		for(; output != outputs.end(); ++output) {
			ret.push_back((*output));
		}
	}
	return ret;
}

