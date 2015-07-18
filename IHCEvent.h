#ifndef IHCEVENT_H
#define IHCEVENT_H
#include <pthread.h>
#include "IHCIO.h"
#include "IHCServerDefs.h"

class IHCEvent {
public:
        IHCEvent();
        IHCEvent(const IHCEvent& e);
	virtual ~IHCEvent();

        enum IHCServerDefs::Event m_event;
        IHCIO* m_io;
	int getEventNumber() { return m_eventNumber; };
private:
        static int m_eventCounter;
	static pthread_mutex_t m_counterMutex;
        int m_eventNumber;
};

#endif /* IHCEVENT_H */
