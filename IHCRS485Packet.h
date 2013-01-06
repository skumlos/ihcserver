#ifndef IHCRS485PACKET_H
#define IHCRS485PACKET_H
#include "IHCDefs.h"
#include <vector>
#include <ctime>
#include <cstdio>

class IHCRS485Packet {
public:
	IHCRS485Packet(unsigned char id,
		       unsigned char dataType,
                       const std::vector<unsigned char>* data = NULL) :
		m_id(id),
		m_dataType(dataType),
		m_isComplete(false)
	{
		m_packet.push_back(IHCDefs::STX);
		m_packet.push_back(id);
		m_packet.push_back(dataType);
		if(data != NULL) {
			for(unsigned int j = 0; j < data->size(); j++) {
				m_packet.push_back((*data)[j]);
				m_data.push_back((*data)[j]);
			}
		}
		m_packet.push_back(IHCDefs::ETB);
		unsigned int crc = 0;
		for(unsigned int j = 0; j < m_packet.size(); j++) {
			crc += m_packet[j];
		}
		m_packet.push_back((unsigned char)(crc & 0xFF));
		m_isComplete = true;
	};

	IHCRS485Packet(const std::vector<unsigned char>& data) :
		m_isComplete(false)
	{
		unsigned char crc = 0;
		if(data.size() > 2 && data[0] == IHCDefs::STX) {
			m_id = data[1];
			m_dataType = data[2];
			crc += data[0];
			crc += data[1];
			crc += data[2];
			m_packet.push_back(data[0]);
			m_packet.push_back(data[1]);
			m_packet.push_back(data[2]);
			for(unsigned int j = 3; j < data.size(); j++) {
				m_packet.push_back(data[j]);
				crc += data[j];
				if(data[j] != IHCDefs::ETB) {
					m_data.push_back(data[j]);
				} else if(data[j] == IHCDefs::ETB) {
					if((j+1) == (data.size() - 1)) {
						if(data[j+1] == (unsigned char)(crc & 0xFF)) {
							m_packet.push_back(data[j+1]);
							m_isComplete = true;
							break;
						}
					}
				}
			}
		}
	};

	void PrintVector(const std::vector<unsigned char>& data) {
        	for(unsigned int j = 0; j < data.size(); j++) {
                	printf("%.2X ",data[j]);
       		}
       		printf("\n");
	};

	void printTimeStamp(time_t t = 0) {
        	time_t now = (t == 0 ? time(NULL) : t);
        	struct tm* timeinfo;
        	timeinfo = localtime(&now);
        	printf("%.2i:%.2i:%.2i ",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
        	return;
	};

	void print() {
		printTimeStamp();
		std::string receiver;
		switch(m_id) {
			case IHCDefs::ID_MODEM:
				receiver = "MODEM";
			break;
			case IHCDefs::ID_DISPLAY:
				receiver = "DISPLAY";
			break;
			case IHCDefs::ID_IHC:
				receiver = "IHC";
			break;
			case IHCDefs::ID_AC:
				receiver = "AC";
			break;
			case IHCDefs::ID_PC:
				receiver = "PC";
			break;
			case IHCDefs::ID_PC2:
				receiver = "PC2";
			break;
			default:
				receiver = "UNKNOWN";
			break;
		}
		printf("(%s) ",receiver.c_str());
		PrintVector(m_packet);
	};

	~IHCRS485Packet() {
	};

	bool isComplete() { return m_isComplete; };

	std::vector<unsigned char> getPacket() {
		return m_packet;
	};

	std::vector<unsigned char> getData() {
		return m_data;
	};

	unsigned char getID() {
		return m_id;
	};

	unsigned char getDataType() {
		return m_dataType;
	};

private:
	unsigned char m_id;
	unsigned char m_dataType;
	bool m_isComplete;
	std::vector<unsigned char> m_packet;
	std::vector<unsigned char> m_data;
};

#endif /* IHCRS485PACKET_H */
