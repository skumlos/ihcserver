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
 * This is the base class that models I/Os of the IHC system
 *
 * December 2012, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCIO_H
#define IHCIO_H
#include "utils/Subject.h"

class IHCIO : public Subject {
public:
	virtual ~IHCIO(){};
	int getModuleNumber() { return m_moduleNumber; };
	int getIONumber() { return m_ioNumber; };
	bool getState() { return m_state; };

protected:
	IHCIO(int moduleNumber, int ioNumber) :
		m_moduleNumber(moduleNumber),
		m_ioNumber(ioNumber),
		m_state(false)
	{};

        void setState(bool newState) {
                bool oldState = m_state;
                m_state = newState;
                if(m_state != oldState) {
                        notify();
                };
        };

private:

	int m_moduleNumber;
	int m_ioNumber;
	bool m_state;

	friend class IHCInterface;
};

#endif /* IHCIO_H */
