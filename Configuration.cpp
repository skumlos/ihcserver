#include "Configuration.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"
#include <fstream>

Configuration* Configuration::instance = 0;

Configuration* Configuration::getInstance() {
	if(instance == 0) {
		instance = new Configuration();
	}
	return instance;
}

Configuration::Configuration() :
	m_serialDevice("")
{
}

void Configuration::load() throw (bool) {
	std::fstream configFile;
	configFile.open("/etc/ihcserver.cfg",std::fstream::in);
	bool saveConfiguration = false;
	if(configFile.is_open()) {
		try {
			configFile.seekg (0, std::ios::end);
			int length = configFile.tellg();
			if(length == 0) {
				throw false;
			}
  			configFile.seekg (0, std::ios::beg);
			json::UnknownElement obj;
			json::Reader::Read(obj,configFile);
			json::Object conf = (json::Object)obj;
			try {
				json::String serialDevice = conf["serialDevice"];
				m_serialDevice = serialDevice.Value();
				printf("Using serial device %s\n",m_serialDevice.c_str());
			} catch (...) {
				printf("Error parsing serial device from configfile, defaulting to /dev/ttyS0\n");
				m_serialDevice = "/dev/ttyS0";
				saveConfiguration = true;
			}
			try {
				json::Array::const_iterator it;
				json::Object modulesConfiguration = conf["modulesConfiguration"];
				json::Array inputModulesConfiguration = modulesConfiguration["inputModules"];
				for(it = inputModulesConfiguration.Begin(); it != inputModulesConfiguration.End(); it++) {
					json::Object moduleConfig = *it;
					json::Number moduleNumber = moduleConfig["moduleNumber"];
					json::Boolean moduleState = moduleConfig["moduleState"];
					m_moduleStates[IHCServerDefs::INPUTMODULE][moduleNumber.Value()] = moduleState.Value();
				}
				json::Array outputModulesConfiguration = modulesConfiguration["outputModules"];
				for(it = outputModulesConfiguration.Begin(); it != outputModulesConfiguration.End(); it++) {
					json::Object moduleConfig = *it;
					json::Number moduleNumber = moduleConfig["moduleNumber"];
					json::Boolean moduleState = moduleConfig["moduleState"];
					m_moduleStates[IHCServerDefs::OUTPUTMODULE][moduleNumber.Value()] = moduleState.Value();
				}
			} catch(...) {
				printf("Could not find any module configuration, will write default to config file\n");
				throw false;
			}
			configFile.close();
		} catch (...) {
			printf("Could not read configuration from config file, creating default file\n");
			if(m_serialDevice == "") {
				m_serialDevice = "/dev/ttyS0";
			}
			for(unsigned int j = 1; j <= 8; j++) {
				m_moduleStates[IHCServerDefs::INPUTMODULE][j] = true;
			}
			for(unsigned int j = 1; j <= 16; j++) {
				m_moduleStates[IHCServerDefs::OUTPUTMODULE][j] = true;
			}
			saveConfiguration = true;
			configFile.close();
		}
	} else {
		printf("Could not access configuration file\n");
		throw false;
	}
	if(saveConfiguration) {
		printf("Saving default configuration\n");
		save();
		throw false;
	}
}

void Configuration::save() {
	std::fstream configFile;
	configFile.open("/etc/ihcserver.cfg",(std::ios_base::in | std::ios_base::out | std::ios_base::trunc));
	if(configFile.is_open()) {
		json::Object conf;
		conf["serialDevice"] = json::String(m_serialDevice);
		json::Array inputModulesConfiguration;
		json::Array outputModulesConfiguration;
		std::map<enum IHCServerDefs::Type,std::map<int,bool> >::const_iterator it;
		for(it = m_moduleStates.begin(); it != m_moduleStates.end(); it++) {
			if(it->first == IHCServerDefs::INPUTMODULE) {
				std::map<int,bool>::const_iterator it2;
				for(it2 = it->second.begin(); it2 != it->second.end(); it2++) {
					json::Object moduleConfiguration;
					moduleConfiguration["moduleNumber"] = json::Number(it2->first);
					moduleConfiguration["moduleState"] = json::Boolean(it2->second);
					inputModulesConfiguration.Insert(moduleConfiguration);
				}
			}
			if(it->first == IHCServerDefs::OUTPUTMODULE) {
				std::map<int,bool>::const_iterator it2;
				for(it2 = it->second.begin(); it2 != it->second.end(); it2++) {
					json::Object moduleConfiguration;
					moduleConfiguration["moduleNumber"] = json::Number(it2->first);
					moduleConfiguration["moduleState"] = json::Boolean(it2->second);
					outputModulesConfiguration.Insert(moduleConfiguration);
				}
			}
		}
		json::Object modulesConfiguration;
		modulesConfiguration["inputModules"] = inputModulesConfiguration;
		modulesConfiguration["outputModules"] = outputModulesConfiguration;
		conf["modulesConfiguration"] = modulesConfiguration;
//		std::map<enum IHCServerDefs::Type,std::map<int,std::map<int,std::string> > > m_ioDescriptions;
		json::Writer::Write(conf,configFile);
		configFile.close();
	}
}

std::string Configuration::getSerialDevice() {
	return m_serialDevice;
}

bool Configuration::getModuleState(enum IHCServerDefs::Type type, int moduleNumber) {
	return m_moduleStates[type][moduleNumber];
}

void Configuration::setModuleState(enum IHCServerDefs::Type type, int moduleNumber, bool state) {
	m_moduleStates[type][moduleNumber] = state;
	return;
}

std::string Configuration::getIODescription(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber) {
	return m_ioDescriptions[type][moduleNumber][ioNumber];
}

void Configuration::setIODescription(enum IHCServerDefs::Type type,
				     int moduleNumber,
				     int ioNumber,
				     std::string description)
{
	m_ioDescriptions[type][moduleNumber][ioNumber] = description;
	return;
}
