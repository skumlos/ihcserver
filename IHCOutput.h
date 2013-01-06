#ifndef IHCOUTPUT_H
#define IHCOUTPUT_H
#include "IHCIO.h"

class IHCOutput : public IHCIO {
protected:
	IHCOutput(int moduleNumber, int outputNumber) :
		IHCIO(moduleNumber,outputNumber)
	{};
	friend class IHCServer;
};

#endif /* IHCOUTPUT_H */
