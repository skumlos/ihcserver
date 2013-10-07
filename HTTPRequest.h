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
private:
	enum RequestType m_requestType;
	std::string m_requestURI;
	std::string m_payload;
};

#endif /* HTTPREQUEST_H */
