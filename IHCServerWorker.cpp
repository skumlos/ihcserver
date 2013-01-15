#include "IHCServerWorker.h"
#include <string>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include "IHCServer.h"
#include "utils/TCPSocket.h"
#include "IHCServerDefs.h"
#include "3rdparty/cajun-2.0.2/json/reader.h"
#include "3rdparty/cajun-2.0.2/json/writer.h"

IHCServerWorker::IHCServerWorker(TCPSocket* socket,IHCServer* server) :
	Thread(true),
	m_socket(socket),
	m_server(server)
{
	start();
}

IHCServerWorker::~IHCServerWorker() {
	delete m_socket;
}

void IHCServerWorker::thread() {
	try {
		while(1) {
			bool data_available = false;
			while(!data_available) {
				data_available = ((m_socket->poll(1000) == 0) ? true : false);
				if(data_available && m_socket->peek() == 0) {
					throw false;
				}
			}
			std::string buffer;
			m_socket->recv(buffer,4);
			int toReceive = 0;
			toReceive += buffer[3] << 24;
			toReceive += buffer[2] << 16;
			toReceive += buffer[1] << 8;
			toReceive += buffer[0] << 0;
			m_socket->recv(buffer,toReceive);

			std::istringstream ist(buffer);
			json::Object req;
			json::Reader::Read(req,ist);

			json::Object resp;
			std::ostringstream ost;

			try {
				json::String type = req["type"];
				if(type.Value() == "getOutput") {
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					int outputNumber = json::Number(req["ioNumber"]).Value();
					m_socket->send(std::string("ACK"));
					bool state = m_server->getOutputState(moduleNumber,outputNumber);
					resp["type"] = json::String("outputState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["outputNumber"] = json::Number(outputNumber);
					resp["state"] = json::Boolean(state);
				} else if(type.Value() == "toggleOutput") {
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					int outputNumber = json::Number(req["ioNumber"]).Value();
					m_socket->send(std::string("ACK"));
					bool state = m_server->getOutputState(moduleNumber,outputNumber);
					m_server->setOutputState(moduleNumber,outputNumber,!state);
					resp["type"] = json::String("outputState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["outputNumber"] = json::Number(outputNumber);
					resp["state"] = json::Boolean(state);
				} else if(type.Value() == "toggleOutputModule") {
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					m_socket->send(std::string("ACK"));
					m_server->toggleModuleState(IHCServerDefs::OUTPUTMODULE,moduleNumber);
					resp["type"] = json::String("outputModuleState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["state"] = json::Boolean(m_server->getModuleState(IHCServerDefs::OUTPUTMODULE,moduleNumber));
				} else if(type.Value() == "toggleInputModule") {
					int moduleNumber = json::Number(req["moduleNumber"]).Value();
					m_socket->send(std::string("ACK"));
					m_server->toggleModuleState(IHCServerDefs::INPUTMODULE,moduleNumber);
					resp["type"] = json::String("inputModuleState");
					resp["moduleNumber"] = json::Number(moduleNumber);
					resp["state"] = json::Boolean(m_server->getModuleState(IHCServerDefs::INPUTMODULE,moduleNumber));
				} else if(type.Value() == "getAll") {
					m_socket->send(std::string("ACK"));
					resp["type"] = json::String("allModules");
					json::Object modules;
					json::Array inputModules;
					for(unsigned int j = 1; j <= 8; j++) {
						json::Object module;
						module["moduleNumber"] = json::Number(j);
						bool state = m_server->getModuleState(IHCServerDefs::INPUTMODULE,j);
						module["state"] = json::Boolean(state);
						if(state) {
							json::Array inputStates;
							for(unsigned int k = 1; k <= 16; k++) {
								json::Object inputState;
								inputState["inputNumber"] = json::Number(k);
								inputState["inputState"] = json::Boolean(m_server->getInputState(j,k));
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
						bool state = m_server->getModuleState(IHCServerDefs::OUTPUTMODULE,j);
						module["state"] = json::Boolean(state);
						if(state) {
							json::Array outputStates;
							for(unsigned int k = 1; k <= 8; k++) {
								json::Object outputState;
								outputState["outputNumber"] = json::Number(k);
								outputState["outputState"] = json::Boolean(m_server->getOutputState(j,k));
								outputStates.Insert(outputState);
							}
							module["outputStates"] = outputStates;
						}
						outputModules.Insert(module);
					}
					modules["outputModules"] = outputModules;
					resp["modules"] = modules;
				} else if(type.Value() == "saveConfiguration") {
					m_socket->send(std::string("ACK"));
					m_server->saveConfiguration();
				} else {
					throw false;
				}
				json::Writer::Write(resp,ost);
				int stringlength = ost.str().size();
				unsigned char* header = new unsigned char[4];
				header[0] = (unsigned char) (stringlength >> 0);
				header[1] = (unsigned char) (stringlength >> 8);
				header[2] = (unsigned char) (stringlength >> 16);
				header[3] = (unsigned char) (stringlength >> 24);
				m_socket->send(header,4);
				m_socket->send(ost.str());
			} catch (...) {
				printf("Exception in json parsing");
				m_socket->send(std::string("NAK"));
			}
		}
	} catch (...) {
		printf("Exception in socket, probably closed\n");
	}
}

void IHCServerWorker::doCleanup() {
        m_server->deleteServerWorker(this);
}

