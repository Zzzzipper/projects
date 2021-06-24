#include "TestTcpIp.h"

#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

#include <string.h>

TestTcpIp::TestTcpIp(uint16_t recvBufSize, StringBuilder *result, bool humanReadable) :
	observer(NULL),
	result(result),
	recvQueue(recvBufSize),
	recvBuf(NULL),
	recvDataFlag(false),
	humanReadable(humanReadable)
{
}

void TestTcpIp::setObserver(EventObserver *observer) {
	this->observer = observer;
}

bool TestTcpIp::connect(const char *domainname, uint16_t port, Mode mode) {
	*result << "<connect:" << domainname << "," << port << "," << mode << ">";
	return true;
}

bool TestTcpIp::hasRecvData() {
	return recvDataFlag;
}

bool TestTcpIp::send(const uint8_t *data, uint32_t dataLen) {
	*result << "<send=";
	if(humanReadable == false) {
		for(uint16_t i = 0; i < dataLen; i++) {
			result->addHex(data[i]);
		}
	} else {
		for(uint16_t i = 0; i < dataLen; i++) {
			result->add(data[i]);
		}
	}
	*result << ",len=" << dataLen << ">";
	return true;
}

bool TestTcpIp::recv(uint8_t *data, uint32_t size) {
	*result << "<recv=" << size << ">";
#if 0
	if(recvDataFlag == true) {
		sendData();
		return true;
	} else {
		recvBuf = data;
		recvBufSize = size;
		return true;
	}
#else
	recvBuf = data;
	recvBufSize = size;
	return true;
#endif
}

void TestTcpIp::close() {
	*result << "<close>";
}

void TestTcpIp::connectComplete() {
	Event event(TcpIp::Event_ConnectOk);
	observer->proc(&event);
}

void TestTcpIp::connectError() {
	Event event(TcpIp::Event_ConnectError);
	observer->proc(&event);
}

void TestTcpIp::sendComplete() {
	Event event(TcpIp::Event_SendDataOk);
	observer->proc(&event);
}

void TestTcpIp::setRecvDataFlag(bool dataFlag) {
	recvDataFlag = dataFlag;
}

void TestTcpIp::incommingData() {
	Event event(TcpIp::Event_IncomingData);
	observer->proc(&event);
}

bool TestTcpIp::addRecvData(const char *hex, bool recvDataFlag) {
	const char *h = hex;
	for(; *h != '\0'; h++) {
		uint8_t c = 0;
		uint8_t d = hexToDigit(*h);
		if(d > 15) {
			LOG_ERROR(LOG_TEST, "ERROR");
			return false;
		}
		c = (d << 4);
		h++;
		if(*h == '\0') {
			LOG_ERROR(LOG_TEST, "ERROR");
			return false;
		}
		d = hexToDigit(*h);
		if(d > 15) {
			LOG_ERROR(LOG_TEST, "ERROR");
			return false;
		}
		c |= d;
		recvQueue.push(c);
	}
	this->recvDataFlag = recvDataFlag;
	if(recvBuf != NULL) {
		sendData();
		return true;
	} else {
		Event event(TcpIp::Event_IncomingData);
		observer->proc(&event);
		return true;
	}
}

bool TestTcpIp::addRecvString(const char *str, bool recvDataFlag) {
	for(const char *s = str; *s != '\0'; s++) {
		recvQueue.push(*s);
	}
	this->recvDataFlag = recvDataFlag;
	if(recvBuf != NULL) {
		sendData();
		return true;
	} else {
		Event event(TcpIp::Event_IncomingData);
		observer->proc(&event);
		return true;
	}
}

void TestTcpIp::sendData() {
	if(recvQueue.getSize() <= 0) {
		Event event(TcpIp::Event_RecvDataError);
		observer->proc(&event);
		return;
	}

	uint16_t i = 0;
	for(; i < recvBufSize; i++) {
		if(recvQueue.isEmpty() == true) {
			break;
		}
		recvBuf[i] = recvQueue.pop();
	}
	recvBuf = NULL;
	Event event(TcpIp::Event_RecvDataOk, i);
	observer->proc(&event);
	return;
}

void TestTcpIp::remoteClose() {
	Event event(TcpIp::Event_Close);
	observer->proc(&event);
}
