#include "../include/TestHttp.h"

namespace Http {

TestConnection::TestConnection() :
	sendRequestReturn(true),
	closeReturn(true),
	result(4096, 4096)
{
}

void TestConnection::setSendRequestReturn(bool sendRequestReturn) {
	this->sendRequestReturn = sendRequestReturn;
}

void TestConnection::setCloseReturn(bool closeReturn) {
	this->closeReturn = closeReturn;
}

void TestConnection::clearResult() {
	result.clear();
}

const char *TestConnection::getResult() {
	return result.getString();
}

void TestConnection::setObserver(EventObserver *) {
	result << "<setObserver>";
}

bool TestConnection::sendRequest(const Request *req, Response *resp) {
	this->result << "<request>";
	this->req = req;
	this->resp = resp;
	return sendRequestReturn;
}

bool TestConnection::close() {
	result << "<close>";
	return closeReturn;
}

}
