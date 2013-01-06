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
