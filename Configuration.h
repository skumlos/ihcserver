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
