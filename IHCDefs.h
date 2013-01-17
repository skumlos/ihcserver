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
 * These are definitions for the IHCServer protocol
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCDEFS_H
#define IHCDEFS_H

namespace IHCDefs {
	// Transmission bytes
	const unsigned char SOH		= 0x01;
	const unsigned char STX 	= 0x02;
	const unsigned char ACK 	= 0x06;
	const unsigned char ETB 	= 0x17;

	// Commands
	const unsigned char DATA_READY	= 0x30;
	const unsigned char SET_OUTPUT	= 0x7A;
	const unsigned char GET_OUTPUTS = 0x82;
	const unsigned char OUTP_STATE 	= 0x83;
	const unsigned char GET_INPUTS	= 0x86;
	const unsigned char INP_STATE	= 0x87;

	// Receiver IDs
	const unsigned char ID_DISPLAY	= 0x09;
	const unsigned char ID_MODEM 	= 0x0A;
	const unsigned char ID_IHC 	= 0x12;
	const unsigned char ID_AC	= 0x1B;
	const unsigned char ID_PC	= 0x1C;
	const unsigned char ID_PC2	= 0x1D;
};

#endif /* IHCDEFS_H */
