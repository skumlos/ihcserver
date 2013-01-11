#include "Configuration.h"

Configuration* Configuration::instance = 0;

Configuration* Configuration::getInstance() {
	if(instance == 0) {
		instance = new Configuration();
	}
	return instance;
}

Configuration::Configuration() {
}

void Configuration::load() {
	for(unsigned int j = 1; j <= 8; j++) {
		m_moduleStates[IHCServerDefs::INPUTMODULE][j] = true;
	}
	for(unsigned int j = 1; j <= 16; j++) {
		m_moduleStates[IHCServerDefs::OUTPUTMODULE][j] = true;
	}
}

void Configuration::save() {
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
