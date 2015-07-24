#include "Configuration.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"
#include <fstream>
#include <iostream>

pthread_mutex_t Configuration::mutex = PTHREAD_MUTEX_INITIALIZER;
Configuration* Configuration::instance = 0;

Configuration* Configuration::getInstance() {
	pthread_mutex_lock(&mutex);
	if(instance == 0) {
		instance = new Configuration();
	}
	pthread_mutex_unlock(&mutex);
	return instance;
}

Configuration::Configuration() :
	m_serialDevice(""),
	m_useHWFlowControl(false),
	m_webroot("/var/www")
{
}

void Configuration::load() throw (bool) {
	pthread_mutex_lock(&mutex);
	std::fstream configFile;
	configFile.open("/etc/ihcserver.cfg",std::fstream::in);
	bool saveConfiguration = false;
	if(configFile.is_open()) {
		try {
			configFile.seekg (0, std::ios::end);
			int length = configFile.tellg();
			if(length == 0) {
				pthread_mutex_unlock(&mutex);
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
			} catch (std::exception& ex) {
				printf("Error parsing serial device from configfile (%s), defaulting to /dev/ttyS0\n",ex.what());
				m_serialDevice = "/dev/ttyS0";
				saveConfiguration = true;
			}
			try {
				json::Boolean flowControl = conf["useHWFlowControl"];
				m_useHWFlowControl = flowControl.Value();
				printf("%s serial flow control\n",(m_useHWFlowControl ? "Using" : "Not using"));
			} catch (std::exception& ex) {
				printf("Error parsing flow control from configfile (%s), defaulting to false\n",ex.what());
				saveConfiguration = true;
			}
			try {
				json::String webroot = conf["webroot"];
				if(webroot.Value() != "") {
					if(webroot.Value().at(0) == '/') {
						m_webroot = webroot.Value();
					} else {
						printf("webroot is not an absolute path! ");
					}
				}
				printf("Using webroot \"%s\"\n",m_webroot.c_str());
			} catch (std::exception& ex) {
				printf("Error parsing webroot from configfile (%s), defaulting to %s\n",m_webroot.c_str(),ex.what());
				saveConfiguration = true;
			}
			try {
				json::Array variables = conf["variables"];
				json::Array::const_iterator it;
				for(it = variables.Begin(); it != variables.End(); it++) {
					json::Object variable = json::Object(*it);
					std::string key = json::String(variable["key"]).Value();
					std::string value = json::String(variable["value"]).Value();
					m_variables[key] = value;
				}
			} catch (std::exception& ex) {
				printf("No variables found or problem in key/value deciphering (%s)\n",ex.what());
			}
			try {
				json::Array::const_iterator it;
				json::Object modulesConfiguration = conf["modulesConfiguration"];
				json::Array inputModulesConfiguration = modulesConfiguration["inputModules"];
				for(it = inputModulesConfiguration.Begin(); it != inputModulesConfiguration.End(); it++) {
					json::Object moduleConfig = json::Object(*it);
					json::Number moduleNumber = json::Number(moduleConfig["moduleNumber"]);
					json::Boolean moduleState = json::Boolean(moduleConfig["moduleState"]);
					m_moduleStates[IHCServerDefs::INPUTMODULE][moduleNumber.Value()] = moduleState.Value();
					json::Array::const_iterator desc_it;
					try {
						json::Array ioDescriptions = json::Array(moduleConfig["ioDescriptions"]);
						for(desc_it = ioDescriptions.Begin(); desc_it != ioDescriptions.End(); desc_it++) {
							json::Object ioDescription = json::Object(*desc_it);
							json::Number ioNumber = json::Number(ioDescription["ioNumber"]);
							json::String description = json::String(ioDescription["description"]);
							m_ioDescriptions[IHCServerDefs::INPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = description.Value();
							json::Boolean isProtected = json::Boolean(ioDescription["protected"]);
							m_ioProtected[IHCServerDefs::INPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = isProtected.Value();
							json::Boolean isAlarm = json::Boolean(ioDescription["alarm"]);
							m_ioAlarm[IHCServerDefs::INPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = isAlarm.Value();
							json::Boolean isEntry = json::Boolean(ioDescription["entry"]);
							m_ioEntry[IHCServerDefs::INPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = isEntry.Value();
						}
					} catch(std::exception& ex) {
						printf("No I/O definitions found in configuration (%s)\n",ex.what());
					}
				}
				json::Array outputModulesConfiguration = modulesConfiguration["outputModules"];
				for(it = outputModulesConfiguration.Begin(); it != outputModulesConfiguration.End(); it++) {
					json::Object moduleConfig = json::Object(*it);
					json::Number moduleNumber = json::Number(moduleConfig["moduleNumber"]);
					json::Boolean moduleState = json::Boolean(moduleConfig["moduleState"]);
					m_moduleStates[IHCServerDefs::OUTPUTMODULE][moduleNumber.Value()] = moduleState.Value();
					json::Array::const_iterator desc_it;
					try {
						json::Array ioDescriptions = json::Array(moduleConfig["ioDescriptions"]);
						for(desc_it = ioDescriptions.Begin(); desc_it != ioDescriptions.End(); desc_it++) {
							json::Object ioDescription = json::Object(*desc_it);
							json::Number ioNumber = json::Number(ioDescription["ioNumber"]);
							json::String description = json::String(ioDescription["description"]);
							m_ioDescriptions[IHCServerDefs::OUTPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = description.Value();
							json::Boolean isProtected = json::Boolean(ioDescription["protected"]);
							m_ioProtected[IHCServerDefs::OUTPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = isProtected.Value();
							json::Boolean isAlarm = json::Boolean(ioDescription["alarm"]);
							m_ioAlarm[IHCServerDefs::OUTPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = isAlarm.Value();
							json::Boolean isEntry = json::Boolean(ioDescription["entry"]);
							m_ioEntry[IHCServerDefs::OUTPUTMODULE][moduleNumber.Value()][ioNumber.Value()] = isEntry.Value();
						}
					} catch(std::exception& ex) {
						printf("No I/O definitions found in configuration (%s)\n",ex.what());
					}
				}
			} catch(std::exception& ex) {
				printf("Could not find any module configuration, will write default to config file (%s)\n",ex.what());
				pthread_mutex_unlock(&mutex);
				throw false;
			}
			configFile.close();
		} catch (bool ex) {
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
		pthread_mutex_unlock(&mutex);
		throw false;
	}
	if(saveConfiguration) {
		printf("Saving default configuration\n");
		pthread_mutex_unlock(&mutex);
		save();
		throw false;
	}
	pthread_mutex_unlock(&mutex);
}

void Configuration::save() {
	pthread_mutex_lock(&mutex);
	std::fstream configFile;
	configFile.open("/etc/ihcserver.cfg",(std::ios_base::in | std::ios_base::out | std::ios_base::trunc));
	if(configFile.is_open()) {
		json::Object conf;
		conf["serialDevice"] = json::String(m_serialDevice);
		conf["useHWFlowControl"] = json::Boolean(m_useHWFlowControl);
		conf["webroot"] = json::String(m_webroot);
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
					json::Array ioDescriptions;
					std::map<int,std::string>::const_iterator desc_it;
					for(desc_it = m_ioDescriptions[IHCServerDefs::INPUTMODULE][it2->first].begin();
						desc_it != m_ioDescriptions[IHCServerDefs::INPUTMODULE][it2->first].end();
						desc_it++) {
						json::Object ioDescription;
						ioDescription["ioNumber"] = json::Number(desc_it->first);
						ioDescription["description"] = json::String(desc_it->second);
						ioDescription["protected"] = json::Boolean(m_ioProtected[IHCServerDefs::INPUTMODULE][it2->first][desc_it->first]);
						ioDescription["alarm"] = json::Boolean(m_ioAlarm[IHCServerDefs::INPUTMODULE][it2->first][desc_it->first]);
						ioDescription["entry"] = json::Boolean(m_ioEntry[IHCServerDefs::INPUTMODULE][it2->first][desc_it->first]);
						ioDescriptions.Insert(ioDescription);
					}
					moduleConfiguration["ioDescriptions"] = ioDescriptions;
					inputModulesConfiguration.Insert(moduleConfiguration);
				}
			}
			if(it->first == IHCServerDefs::OUTPUTMODULE) {
				std::map<int,bool>::const_iterator it2;
				for(it2 = it->second.begin(); it2 != it->second.end(); it2++) {
					json::Object moduleConfiguration;
					moduleConfiguration["moduleNumber"] = json::Number(it2->first);
					moduleConfiguration["moduleState"] = json::Boolean(it2->second);
					json::Array ioDescriptions;
					std::map<int,std::string>::const_iterator desc_it;
					for(desc_it = m_ioDescriptions[IHCServerDefs::OUTPUTMODULE][it2->first].begin();
						desc_it != m_ioDescriptions[IHCServerDefs::OUTPUTMODULE][it2->first].end();
						desc_it++) {
						json::Object ioDescription;
						ioDescription["ioNumber"] = json::Number(desc_it->first);
						ioDescription["description"] = json::String(desc_it->second);
						ioDescription["protected"] = json::Boolean(m_ioProtected[IHCServerDefs::OUTPUTMODULE][it2->first][desc_it->first]);
						ioDescription["alarm"] = json::Boolean(m_ioAlarm[IHCServerDefs::OUTPUTMODULE][it2->first][desc_it->first]);
						ioDescription["entry"] = json::Boolean(m_ioEntry[IHCServerDefs::OUTPUTMODULE][it2->first][desc_it->first]);
						ioDescriptions.Insert(ioDescription);
					}
					moduleConfiguration["ioDescriptions"] = ioDescriptions;
					outputModulesConfiguration.Insert(moduleConfiguration);
				}
			}
		}
		json::Object modulesConfiguration;
		modulesConfiguration["inputModules"] = inputModulesConfiguration;
		modulesConfiguration["outputModules"] = outputModulesConfiguration;
		conf["modulesConfiguration"] = modulesConfiguration;

		json::Array jsonVariables;
		std::map<std::string,std::string>::iterator vit = m_variables.begin();
		for(; vit != m_variables.end(); vit++) {
			json::Object variable;
			variable["key"] = json::String(vit->first);
			variable["value"] = json::String(vit->second);
			jsonVariables.Insert(variable);
		}
		conf["variables"] = jsonVariables;

		json::Writer::Write(conf,configFile);
		configFile.close();
	}
	pthread_mutex_unlock(&mutex);
}

std::string Configuration::getValue(std::string variable) {
	return m_variables[variable];
}

void Configuration::setValue(std::string variable, std::string value) {
	m_variables[variable] = value;
	return;
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

bool Configuration::getIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber) {
	return m_ioProtected[type][moduleNumber][ioNumber];
}

void Configuration::setIOProtected(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isProtected) {
	m_ioProtected[type][moduleNumber][ioNumber] = isProtected;
}

bool Configuration::getIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber) {
	return m_ioAlarm[type][moduleNumber][ioNumber];
}

void Configuration::setIOAlarm(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isAlarm) {
	m_ioAlarm[type][moduleNumber][ioNumber] = isAlarm;
}

bool Configuration::getIOEntry(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber) {
	return m_ioEntry[type][moduleNumber][ioNumber];
}

void Configuration::setIOEntry(enum IHCServerDefs::Type type, int moduleNumber, int ioNumber, bool isEntry) {
	m_ioEntry[type][moduleNumber][ioNumber] = isEntry;
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
