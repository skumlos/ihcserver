#include "IHCServerWorker.h"
#include "IHCServer.h"
#include "Configuration.h"
#include "IHCServerDefs.h"
#include "IHCEvent.h"
#include "IHCIO.h"
#include "Userlevel.h"
#include "utils/ms.h"
#include <errno.h>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <openssl/sha.h>
#include "3rdparty/cajun-2.0.2/json/elements.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"

IHCServerWorker::IHCServerWorker() : Thread(true) {}

IHCServerWorker::~IHCServerWorker() {}

void IHCServerWorker::getModuleConfiguration(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& resp)
{
	IHCServer* ihcserver = IHCServer::getInstance();
	if(Userlevel::getUserlevel(token) != Userlevel::ADMIN) {
		printf("IHCServerWorker: Client trying to getModuleConfiguration without proper level\n");
		throw false;
	}
	std::string moduleType = json::String(req["moduleType"]).Value();
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	resp["type"] = json::String("moduleConfiguration");
	resp["moduleNumber"] = json::Number(moduleNumber);
	resp["moduleType"] = json::String(moduleType);
	enum IHCServerDefs::Type type;
	int ios = 0;
	if(moduleType == "input") {
		type = IHCServerDefs::INPUTMODULE;
		ios = 16;
	} else if(moduleType == "output") {
		type = IHCServerDefs::OUTPUTMODULE;
		ios = 8;
	} else {
		throw false;
	}
	resp["state"] = json::Boolean(ihcserver->getModuleState(type,moduleNumber));
	json::Array ioDescriptions;
	for(int j = 1; j <= ios; j++) {
		json::Object ioDescription;
		if(ihcserver->getIODescription(type,moduleNumber,j) != "") {
			ioDescription["ioNumber"] = json::Number(j);
			ioDescription["ioDescription"] = json::String(ihcserver->getIODescription(type,moduleNumber,j));
			ioDescriptions.Insert(ioDescription);
		}
	}
	resp["ioDescriptions"] = ioDescriptions;
	json::Array ioProtectedStates;
	for(int j = 1; j <= ios; j++) {
		json::Object ioProtected;
		if(ihcserver->getIOProtected(type,moduleNumber,j)) {
			ioProtected["ioNumber"] = json::Number(j);
			ioProtected["ioProtected"] = json::Boolean(true);
			ioProtectedStates.Insert(ioProtected);
		}
	}
	resp["ioProtectedStates"] = ioProtectedStates;
	json::Array ioAlarmStates;
	for(int j = 1; j <= ios; j++) {
		json::Object ioAlarm;
		if(ihcserver->getIOAlarm(type,moduleNumber,j)) {
			ioAlarm["ioNumber"] = json::Number(j);
			ioAlarm["ioAlarm"] = json::Boolean(true);
			ioAlarmStates.Insert(ioAlarm);
		}
	}
	resp["ioAlarmStates"] = ioAlarmStates;
	return;
}

void IHCServerWorker::setModuleConfiguration(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& resp)
{
	IHCServer* ihcserver = IHCServer::getInstance();
	if(Userlevel::getUserlevel(token) != Userlevel::ADMIN) {
		printf("IHCServerWorker: Client trying to setModuleConfiguration without proper level\n");
		throw false;
	}
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	std::string moduleType = json::String(req["moduleType"]).Value();
	enum IHCServerDefs::Type type;
	int ios = 0;
	if(moduleType == "input") {
		type = IHCServerDefs::INPUTMODULE;
	} else if(moduleType == "output") {
		type = IHCServerDefs::OUTPUTMODULE;
	} else {
		throw false;
	}

	json::Array::const_iterator it;
	json::Array ioDescriptions = json::Array(req["ioDescriptions"]);
	for(it = ioDescriptions.Begin(); it != ioDescriptions.End(); it++) {
		json::Object ioDescriptionObj = json::Object(*it);
		int ioNumber = json::Number(ioDescriptionObj["ioNumber"]).Value();
		std::string ioDescription = json::String(ioDescriptionObj["ioDescription"]).Value();
		if(ioDescription != "") {
			ihcserver->setIODescription(type,moduleNumber,ioNumber,ioDescription);
		}
	}
	json::Array ioProtectedStates = json::Array(req["ioProtectedStates"]);
	for(it = ioProtectedStates.Begin(); it != ioProtectedStates.End(); it++) {
		json::Object ioProtectedObj = json::Object(*it);
		int ioNumber = json::Number(ioProtectedObj["ioNumber"]).Value();
		bool ioProtected = json::Boolean(ioProtectedObj["ioProtected"]).Value();
		ihcserver->setIOProtected(type,moduleNumber,ioNumber,ioProtected);
	}
	json::Array ioAlarmStates = json::Array(req["ioAlarmStates"]);
	for(it = ioAlarmStates.Begin(); it != ioAlarmStates.End(); it++) {
		json::Object ioAlarmObj = json::Object(*it);
		int ioNumber = json::Number(ioAlarmObj["ioNumber"]).Value();
		bool ioAlarm = json::Boolean(ioAlarmObj["ioAlarm"]).Value();
		ihcserver->setIOAlarm(type,moduleNumber,ioNumber,ioAlarm);
	}
	bool moduleState = json::Boolean(req["state"]).Value();
	if(moduleState != ihcserver->getModuleState(type,moduleNumber)) {
		ihcserver->toggleModuleState(type,moduleNumber);
	}
	ihcserver->saveConfiguration();
	resp["result"] = json::Boolean(true);
	return;
}

void IHCServerWorker::toggleOutput(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& response) {
	IHCServer* ihcserver = IHCServer::getInstance();
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	int outputNumber = json::Number(req["ioNumber"]).Value();
	if(ihcserver->getIOProtected(IHCServerDefs::OUTPUTMODULE,moduleNumber,outputNumber) &&
	   (Userlevel::getUserlevel(token) != Userlevel::ADMIN && Userlevel::getUserlevel(token) != Userlevel::SUPERUSER))
	{
		throw false;
	}
	bool state = ihcserver->getOutputState(moduleNumber,outputNumber);
	ihcserver->setOutputState(moduleNumber,outputNumber,!state);
	response["type"] = json::String("outputState");
	response["moduleNumber"] = json::Number(moduleNumber);
	response["outputNumber"] = json::Number(outputNumber);
	response["state"] = json::Boolean(state);
	return;
}

void IHCServerWorker::activateInput(Userlevel::UserlevelToken* &token, json::Object& req, bool shouldActivate, json::Object& response) {
	IHCServer* ihcserver = IHCServer::getInstance();
	int moduleNumber = json::Number(req["moduleNumber"]).Value();
	int outputNumber = json::Number(req["ioNumber"]).Value();
/*	if(ihcserver->getIOProtected(IHCServerDefs::OUTPUTMODULE,moduleNumber,outputNumber) &&
	   (Userlevel::getUserlevel(token) != Userlevel::ADMIN && Userlevel::getUserlevel(token) != Userlevel::SUPERUSER))
	{
		throw false;
	}*/
	ihcserver->activateInput(moduleNumber,outputNumber,shouldActivate);
	return;
}

void IHCServerWorker::getAlarmState(json::Object& response) {
	IHCServer* ihcserver = IHCServer::getInstance();
	response["type"] = json::String("getAlarmState");
	response["state"] = json::Boolean(ihcserver->getAlarmState());
	return;
}

void IHCServerWorker::getAll(json::Object& resp) {
	IHCServer* ihcserver = IHCServer::getInstance();
	resp["type"] = json::String("allModules");
	json::Object modules;
	json::Array inputModules;
	for(unsigned int j = 1; j <= 8; j++) {
		json::Object module;
		module["moduleNumber"] = json::Number(j);
		bool state = ihcserver->getModuleState(IHCServerDefs::INPUTMODULE,j);
		module["state"] = json::Boolean(state);
		if(state) {
			json::Array inputStates;
			for(unsigned int k = 1; k <= 16; k++) {
				json::Object inputState;
				inputState["inputNumber"] = json::Number(k);
				inputState["inputState"] = json::Boolean(ihcserver->getInputState(j,k));
				inputState["description"] = json::String(ihcserver->getIODescription(IHCServerDefs::INPUTMODULE,j,k));
				inputState["protected"] = json::Boolean(ihcserver->getIOProtected(IHCServerDefs::INPUTMODULE,j,k));
				inputStates.Insert(inputState);
			}
			module["inputStates"] = inputStates;
		}
		inputModules.Insert(module);
	}
	modules["inputModules"] = inputModules;
	json::Array outputModules;
	for(unsigned int j = 1; j <= 16; j++) {
		json::Object module;
		module["moduleNumber"] = json::Number(j);
		bool state = ihcserver->getModuleState(IHCServerDefs::OUTPUTMODULE,j);
		module["state"] = json::Boolean(state);
		if(state) {
			json::Array outputStates;
			for(unsigned int k = 1; k <= 8; k++) {
				json::Object outputState;
				outputState["outputNumber"] = json::Number(k);
				outputState["outputState"] = json::Boolean(ihcserver->getOutputState(j,k));
				outputState["description"] = json::String(ihcserver->getIODescription(IHCServerDefs::OUTPUTMODULE,j,k));
				outputState["protected"] = json::Boolean(ihcserver->getIOProtected(IHCServerDefs::OUTPUTMODULE,j,k));
				outputStates.Insert(outputState);
			}
			module["outputStates"] = outputStates;
		}
		outputModules.Insert(module);
	}
	modules["outputModules"] = outputModules;
	resp["modules"] = modules;
	return;
}

void IHCServerWorker::keypadAction(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& response)
{
	std::string action = json::String(req["action"]).Value();
	std::string input = json::String(req["input"]).Value();
	if(action == "setAdminCode") {
		if(input != "") {
			if(Userlevel::getUserlevel(token) == Userlevel::ADMIN) {
				Userlevel::setCodeSHA(Userlevel::ADMIN,input);
			} else {
				throw false;
			}
		}
	} else if(action == "setSuperUserCode") {
		if(input != "") {
			if(Userlevel::getUserlevel(token) == Userlevel::SUPERUSER ||
				Userlevel::getUserlevel(token) == Userlevel::ADMIN) {
				Userlevel::setCodeSHA(Userlevel::SUPERUSER,input);
			} else {
				throw false;
			}
		}
	} else if(action == "login") {
		try {
			Userlevel::loginSHA(token,input);
		} catch(std::exception& ex) {
			printf("IHCServerWorker: Exception when logging in (%s)\n",ex.what());
		} catch(bool ex) {
			printf("IHCServerWorker: Unknown exception when logging in\n");
		}
		response["Userlevel"] = json::String(Userlevel::tokenToString(token));
	} else if(action == "arm-alarm") {
		Userlevel::UserlevelToken *tempToken;
		try {
			Userlevel::loginSHA(tempToken,input);
		} catch(std::exception& ex) {
			printf("IHCServerWorker: Exception when logging in (%s)\n",ex.what());
		} catch(bool ex) {
			printf("IHCServerWorker: Unknown exception when logging in\n");
		}
		if(Userlevel::getUserlevel(tempToken) == Userlevel::BASIC) {
			printf("IHCServerWorker: Client trying to arm without proper level\n");
			throw false;
		}
		IHCServer::getInstance()->setAlarmState(true);
	} else if(action == "disarm-alarm") {
		Userlevel::UserlevelToken *tempToken;
		try {
			Userlevel::loginSHA(tempToken,input);
		} catch(std::exception& ex) {
			printf("IHCServerWorker: Exception when logging in (%s)\n",ex.what());
		} catch(bool ex) {
			printf("IHCServerWorker: Unknown exception when logging in\n");
		}
		if(Userlevel::getUserlevel(tempToken) == Userlevel::BASIC) {
			printf("IHCServerWorker: Client trying to disarm without proper level\n");
			throw false;
		}
		IHCServer::getInstance()->setAlarmState(false);
	}
	response["result"] = json::Boolean(true);
}

void IHCServerWorker::getUserlevel(Userlevel::UserlevelToken* &token, json::Object& req, json::Object& response) {
	response["type"] = json::String("getUserlevel");
	response["Userlevel"] = json::String(Userlevel::tokenToString(token));
}
