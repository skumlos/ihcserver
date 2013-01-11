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
	m_socket(socket),
	m_server(server)
{
	start();
}

void IHCServerWorker::thread() {
	try {
	while(1) {
		printf("Waiting for request\n");
		m_socket->poll(-1);
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
				printf("getOutput\n");
				int moduleNumber = json::Number(req["moduleNumber"]).Value();
				int outputNumber = json::Number(req["outputNumber"]).Value();
				m_socket->send(std::string("ACK"));
				bool state = m_server->getOutputState(moduleNumber,outputNumber);
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
			} else {
				throw false;
			}
			json::Writer::Write(resp,ost);
			int stringlength = ost.str().size();
			unsigned char* header = new unsigned char[4];
			header[0] = (unsigned char) (stringlength << 0);
			header[1] = (unsigned char) (stringlength << 8);
			header[2] = (unsigned char) (stringlength << 16);
			header[3] = (unsigned char) (stringlength << 24);
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
