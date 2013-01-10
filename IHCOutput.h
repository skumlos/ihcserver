#ifndef IHCOUTPUT_H
#define IHCOUTPUT_H
#include "IHCIO.h"

class IHCOutput : public IHCIO {
public:
	int getOutputNumber() { return getIONumber(); };
protected:
	IHCOutput(int moduleNumber, int outputNumber) :
		IHCIO(moduleNumber,outputNumber)
	{};
	friend class IHCInterface;
};

#endif /* IHCOUTPUT_H */
