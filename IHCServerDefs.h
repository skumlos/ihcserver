/*
 * These are definitions that are specific to IHCServer
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCSERVERDEFS_H
#define IHCSERVERDEFS_H

namespace IHCServerDefs {
        enum Type {
                INPUT,
                OUTPUT,
                INPUTMODULE,
                OUTPUTMODULE
        };

	enum Event {
		UNKNOWN,
		INPUT_CHANGED,
		OUTPUT_CHANGED,
		ALARM_ARMED,
		ALARM_DISARMED,
		ENTRY
	};

	const static unsigned int IO_FLAG_ALARM = 1;
	const static unsigned int IO_FLAG_PROTECTED = 2;
	const static unsigned int IO_FLAG_ENTRY = 2;
};

#endif /* IHCSERVERDEFS_H */
