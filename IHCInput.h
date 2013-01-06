#ifndef IHCINPUT_H
#define IHCINPUT_H
#include "IHCIO.h"

class IHCInput : public IHCIO {
public:
	int getInputNumber() { return getIONumber(); };
protected:
	IHCInput(int moduleNumber, int inputNumber) :
		IHCIO(moduleNumber, inputNumber)
	{};
	friend class IHCServer;
};

#endif /* IHCINPUT_H */
