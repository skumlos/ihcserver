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
