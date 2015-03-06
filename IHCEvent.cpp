#include "IHCEvent.h"
int IHCEvent::m_eventCounter = 0;
pthread_mutex_t IHCEvent::m_counterMutex = PTHREAD_MUTEX_INITIALIZER;

IHCEvent::IHCEvent() :
	m_event(IHCServerDefs::UNKNOWN),
	m_io(NULL)
{
	pthread_mutex_lock(&m_counterMutex);
	m_eventCounter++;
	m_eventNumber = m_eventCounter;
	pthread_mutex_unlock(&m_counterMutex);
};

IHCEvent::IHCEvent(const IHCEvent& e) :
	m_event(e.m_event),
	m_io(e.m_io)
{};

