#include "IHCServerWorker.h"
#include <string>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include "IHCServer.h"
#include "utils/TCPSocket.h"
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
		m_socket->poll(-1);
		std::string buffer;
		m_socket->recv(buffer,4);
		int toReceive = 0;
		toReceive += buffer[3] << 24;
		toReceive += buffer[2] << 16;
		toReceive += buffer[1] << 8;
		toReceive += buffer[0] << 0;
		printf("Should receive %d bytes\n", toReceive);
		m_socket->recv(buffer,toReceive);
		printf("%s\n",buffer.c_str());
		std::istringstream ist(buffer);
		json::Object obj;
		json::Reader::Read(obj,ist);
		json::Object resp;
		try {
			std::ostringstream ost;
			if(json::String(obj["type"]).Value() == "getOutput") {
				int moduleNumber = json::Number(obj["moduleNumber"]).Value();
				int outputNumber = json::Number(obj["outputNumber"]).Value();
				m_socket->send(std::string("ACK"));
				bool state = m_server->getOutputState(moduleNumber,outputNumber);
				resp["type"] = json::String("outputState");
				resp["moduleNumber"] = json::Number(moduleNumber);
				resp["outputNumber"] = json::Number(outputNumber);
				resp["state"] = json::Boolean(state);
				json::Writer::Write(obj,ost);
			}
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
