/**
 * This class is used for applications that want to send requests
 * to IHCServer. It takes in JSON packages, decodes, acts and
 * responds with a JSON packet again (to those requests that needs it)
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#ifndef IHCSERVERREQUESTWORKER_H
#define IHCSERVERREQUESTWORKER_H
#include "IHCServerWorker.h"
#include "Userlevel.h"

class TCPSocket;

class IHCServerRequestWorker : public IHCServerWorker {
public:
	IHCServerRequestWorker(TCPSocket* socket);
	virtual ~IHCServerRequestWorker();
	virtual void thread();
private:
	TCPSocket* m_socket;
	Userlevel::UserlevelToken* m_token;
};

#endif /* IHCSERVERREQUESTWORKER_H */
