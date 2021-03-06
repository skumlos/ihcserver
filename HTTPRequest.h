#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <string>

class HTTPRequest {
public:
	HTTPRequest(const std::string& requestString);

	enum RequestType {
		REQ_UNKNOWN,
		REQ_GET,
		REQ_POST
	};

	enum RequestType getRequestType() { return m_requestType; };
	const std::string& getPayload() { return m_payload; };
	const std::string& getRequestURI() { return m_requestURI; };
	const std::string& getHeader() { return m_header; };
private:
	enum RequestType m_requestType;
	std::string m_requestURI;
	std::string m_payload;
	std::string m_header;
};

#endif /* HTTPREQUEST_H */
