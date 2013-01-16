/**
 * This class controls the configuration of IHCServer
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include <string>
#include <map>
#include "IHCServerDefs.h"

class Configuration {
public:
	static Configuration* getInstance();
	std::string getSerialDevice();
	bool getModuleState(enum IHCServerDefs::Type type, int outputModuleNumber);
	void setModuleState(enum IHCServerDefs::Type type, int inputModuleNumber, bool state);
	std::string getIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber);
	void setIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, std::string description);

	void load() throw (bool);
	void save();

private:
	Configuration();
	static Configuration* instance;
	std::string m_serialDevice;
	std::map<enum IHCServerDefs::Type,std::map<int,bool> > m_moduleStates;
	std::map<enum IHCServerDefs::Type,std::map<int,std::map<int,std::string> > > m_ioDescriptions;
};

#endif /* CONFIGURATION_H */
