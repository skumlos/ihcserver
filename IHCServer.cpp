#include "IHCServer.h"
#include "uart/Uart.h"
#include <cstdlib>
#include <ctime>

void printTimeStamp(time_t t = 0) {
        time_t now = (t == 0 ? time(NULL) : t);
        struct tm* timeinfo;
        timeinfo = localtime(&now);
        printf("%.2i:%.2i:%.2i ",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
        return;
};

IHCServer::IHCServer(std::string rs485port)
{
	try {
		m_port = new UART(rs485port,true);
		m_port->setSpeed(B19200);
	} catch (...) {
		throw false;
	}

	for(unsigned int j = 0; j < 16; j++) {
		std::vector<IHCOutput*> v;
		for(unsigned int k = 0; k < 8; k++) {
			IHCOutput* output = new IHCOutput(j+1,k+1);
			v.push_back(output);
		}
		m_outputs[j] = v;
	}
	for(unsigned int j = 0; j < 8; j++) {
		std::vector<IHCInput*> v;
		for(unsigned int k = 0; k < 16; k++) {
			IHCInput* input = new IHCInput(j+1,k+1);
			v.push_back(input);
		}
		m_inputs[j] = v;
	}
	start();
}

IHCServer::~IHCServer() {
}

void IHCServer::changeOutput(IHCOutput* output, bool newState) {
}

void IHCServer::updateInputStates(const std::vector<unsigned char>& newStates) {
	for(unsigned int k = 0; k < 8; k++) {
		unsigned int ioModuleState = newStates[(k*2)] + (newStates[(k*2)+1] << 8);
		for(unsigned int j = 0; j < 16; j++) {
			bool state = (((ioModuleState & (1 << j)) > 0) ? true : false);
			if(m_inputs[k][j]->getState() != state) {
				if(state) {
					printTimeStamp();
					printf("Input %d.%d is ON\n",k+1,j+1);
				} else {
					printTimeStamp();
					printf("Input %d.%d is OFF\n",k+1,j+1);
				}
				m_inputs[k][j]->setState(state);
			}
		}
	}
}

void IHCServer::updateOutputStates(const std::vector<unsigned char>& newStates) {
	for(unsigned int k = 0; k < newStates.size(); k++) {
		unsigned char ioModuleState = newStates[k];
		for(unsigned int j = 0; j < 8; j++) {
			bool state = (((ioModuleState & (1 << j)) > 0) ? true : false);
			if(m_outputs[k][j]->getState() != state) {
				if(state) {
					printTimeStamp();
					printf("Output %d.%d is ON\n",k+1,j+1);
				} else {
					printTimeStamp();
					printf("Output %d.%d is OFF\n",k+1,j+1);
				}
				m_outputs[k][j]->setState(state);
			}
		}
	}
}

IHCRS485Packet IHCServer::getPacket(UART& uart, int ID, bool useTimeout) throw (bool) {
	unsigned char c = 0;
	std::vector<unsigned char> packet;
	uart.poll();
	time_t timeout_s = time(NULL) + 5;
	int out_of_frame_bytes = 0;
	while(1) {
		out_of_frame_bytes = 0;
		c = uart.readByte();
		while(c != IHCDefs::STX) { // Syncing with STX
//			printf("%.2X ",c);
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
//			p.print();
			return p;
		}
		packet.clear();
		if(useTimeout && time(NULL) >= timeout_s) throw false;
	}
};

void IHCServer::thread() {
	std::vector<unsigned char> outputBlocks(16,0x0);
	std::vector<unsigned char> inputBlocks(16,0x0);

	int id = IHCDefs::ID_PC;

	time_t begin = time(NULL);

	IHCRS485Packet getInputPacket(IHCDefs::ID_IHC, IHCDefs::GET_INPUTS);
	IHCRS485Packet getOutputPacket(IHCDefs::ID_IHC, IHCDefs::GET_OUTPUTS);

	int lastUpdated = IHCDefs::INP_STATE;
	std::vector<unsigned char> io;

	while(1) {
		try {
			IHCRS485Packet packet = getPacket(*m_port,id);
			switch(packet.getDataType()) {
				case IHCDefs::DATA_READY:
					if(lastUpdated == IHCDefs::INP_STATE) {
						m_port->write(getOutputPacket.getPacket());
					} else {
						m_port->write(getInputPacket.getPacket());
					}
				break;
				case IHCDefs::INP_STATE:
					io = packet.getData();
					updateInputStates(io);
					lastUpdated = IHCDefs::INP_STATE;
				break;
				case IHCDefs::OUTP_STATE:
					io = packet.getData();
					updateOutputStates(io);
					lastUpdated = IHCDefs::OUTP_STATE;
				break;
			}
		} catch (...) {
			printf("Caught exception in communication thread\n");
			exit(1);
		}
	}
}

IHCInput* IHCServer::getInput(int moduleNumber, int inputNumber) {
	if(moduleNumber < 1 || moduleNumber > 8 || inputNumber > 16 || inputNumber < 1) {
		return NULL;
	}
	return m_inputs[moduleNumber][inputNumber];
}

IHCOutput* IHCServer::getOutput(int moduleNumber, int outputNumber) {
	if(moduleNumber < 1 || moduleNumber > 16 || outputNumber > 8 || outputNumber < 1) {
		return NULL;
	}
	return m_outputs[moduleNumber][outputNumber];
}

