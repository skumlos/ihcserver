#include "HTTPRequest.h"
#include <cstdio>
#include <locale>

HTTPRequest::HTTPRequest(const std::string& requestString) :
	m_requestType(REQ_UNKNOWN),
	m_requestURI(""),
	m_payload(""),
	m_header("")
{
//	printf("req: ..%s..\n",requestString.c_str());
	size_t pos1 = std::string::npos;
	pos1 = requestString.find("\r\n");
	if(pos1 != std::string::npos) {
		std::string requestHeader = requestString.substr(0,pos1);
		if((pos1 = requestHeader.find(' ')) != std::string::npos) {
			std::string type = requestHeader.substr(0,pos1);
			std::string type_lc = "";
			for(unsigned int j = 0; j < type.size(); j++) {
				type_lc += std::tolower(type[j]);
			}
			if(type_lc == "get") {
				m_requestType = REQ_GET;
			} else if (type_lc == "post") {
				m_requestType = REQ_POST;
			}
		}
		size_t pos2 = requestHeader.substr(pos1+1).find(' ');
		if(pos2 != std::string::npos) {
			m_requestURI = requestHeader.substr(pos1+1,pos2);
		}
	} else {
		throw false;
	}
	pos1 = requestString.find("\r\n\r\n");
	if(pos1 != std::string::npos) {
		std::string httpHeader = "";
		std::string httpHeaderTmp = requestString.substr(0,pos1+4);
		m_header = httpHeaderTmp;
		for(unsigned int j = 0; j < httpHeaderTmp.size(); j++) {
			httpHeader += std::tolower(httpHeaderTmp[j]);
		}
		size_t pos2 = httpHeader.find("content-length:");
		if(pos2 != std::string::npos) {
			size_t pos3 = httpHeader.find("\r\n",pos2);
			if(pos3 != std::string::npos) {
//				printf("content length .%s.\n", httpHeader.substr(pos2,pos3-pos2).c_str());
			}
		}
		m_payload = requestString.substr(pos1+4);
	}
}
