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
	std::list<IHCInput*> getAllInputs();
	std::list<IHCOutput*> getAllOutputs();
	void changeOutput(IHCOutput* output, bool newState);
	void changeInput(IHCInput* input, bool shouldActivate);
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
