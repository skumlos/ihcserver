#ifndef IHCINPUT_H
#define IHCINPUT_H
#include "IHCIO.h"

class IHCInput : public IHCIO {
public:
	IHCInput(int moduleNumber, int inputNumber) :
		IHCIO(moduleNumber, inputNumber)
	{};
};

#endif /* IHCINPUT_H */
