/**
 * This class controls the configuration of IHCServer.
 * Configuration is save as a JSON encoded object in
 * /etc/ihcserver.cfg
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include "IHCServerDefs.h"
#include <string>
#include <map>
#include <pthread.h>

class Configuration {
public:
	// Configuration is a singleton
	static Configuration* getInstance();

	// Interface for the configuration
	std::string getSerialDevice();
	bool useHWFlowControl() { return m_useHWFlowControl; };

	std::string getWebroot() { return m_webroot; };

	bool getModuleState(enum IHCServerDefs::Type type, int outputModuleNumber);
	void setModuleState(enum IHCServerDefs::Type type, int inputModuleNumber, bool state);
	std::string getIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);
	void setIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, std::string description);

	bool getIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);
	void setIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isProtected);

	bool getIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);
	void setIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isAlarm);

	bool getIOEntry(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);
	void setIOEntry(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isEntry);

	// More generic interface
	std::string getValue(std::string variable);
	void setValue(std::string variable, std::string value);

	void load() throw (bool);
	void save();

private:
	Configuration();
	static Configuration* instance;
	static pthread_mutex_t mutex;
	std::string m_serialDevice;
	bool m_useHWFlowControl;
	std::string m_webroot;
	std::map<enum IHCServerDefs::Type,std::map<int,bool> > m_moduleStates;
	std::map<enum IHCServerDefs::Type,std::map<int,std::map<int,std::string> > > m_ioDescriptions;
	std::map<enum IHCServerDefs::Type,std::map<int,std::map<int,bool> > > m_ioProtected;
	std::map<enum IHCServerDefs::Type,std::map<int,std::map<int,bool> > > m_ioAlarm;
	std::map<enum IHCServerDefs::Type,std::map<int,std::map<int,bool> > > m_ioEntry;

	std::map<std::string,std::string> m_variables;
};

#endif /* CONFIGURATION_H */
