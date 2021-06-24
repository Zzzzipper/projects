#ifndef COMMON_HTTP_TESTHTTP_H
#define COMMON_HTTP_TESTHTTP_H

#include "Http.h"

namespace Http {

class TestConnection : public ClientInterface {
public:
	TestConnection();
	void setSendRequestReturn(bool sendRequestReturn);
	void setCloseReturn(bool closeReturn);
	void clearResult();
	const char *getResult();
	const Request *getRequest() { return req; }
	Response *getResponse() { return resp; }

	virtual void setObserver(EventObserver *);
	virtual bool sendRequest(const Request *req, Response *resp);
	virtual bool close();

private:
	bool sendRequestReturn;
	bool closeReturn;
	StringBuilder result;
	const Request *req;
	Response *resp;
};

}

#endif
