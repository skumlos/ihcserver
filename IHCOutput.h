#ifndef IHCOUTPUT_H
#define IHCOUTPUT_H
#include "IHCIO.h"

class IHCOutput : public IHCIO {
public:
	IHCOutput(int moduleNumber, int outputNumber) :
		IHCIO(moduleNumber,outputNumber)
	{};
        void setState(bool newState) {
                oldState = m_state;
                m_state = newState;
                if(m_state != oldState) {
                        //notify();
                };
        };
};

#endif /* IHCOUTPUT_H */
