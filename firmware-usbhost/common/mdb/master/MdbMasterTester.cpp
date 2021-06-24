#include "MdbMasterTester.h"

#include "utils/include/Hex.h"
#include "logger/include/Logger.h"
#include "common.h"

#include <string.h>

void MdbMasterTester::Sender::sendConfirm(Mdb::Control control) {
	buf.clear();
	buf.addUint8(control);
}

void MdbMasterTester::Sender::sendRequest() {
}

uint8_t *MdbMasterTester::Sender::getSendData() {
	return buf.getData();
}

uint16_t MdbMasterTester::Sender::getSendLen() {
	return buf.getLen();
}

void MdbMasterTester::Sender::clearSendData() {
	buf.clear();
}

MdbMasterTester::MdbMasterTester(MdbMaster *master) :
	master(master),
	recvBuffer(256)
{
	master->initMaster(&sender);
}

void MdbMasterTester::poll() {
	master->sendRequest();
}

bool MdbMasterTester::recvData(const char *response) {
	uint16_t len = hexToData(response, strlen(response), recvBuffer.getData(), recvBuffer.getSize());
	if(len == 0) {
		return false;
	}
	sender.clearSendData();
	recvBuffer.setLen(len);
	master->recvResponse(recvBuffer.getData(), recvBuffer.getLen(), true);
	return true;
}

bool MdbMasterTester::recvConfirm(const uint8_t control) {
	sender.clearSendData();
	recvBuffer.clear();
	recvBuffer.addUint8(control);
	master->recvResponse(recvBuffer.getData(), recvBuffer.getLen(), false);
	return true;
}

void MdbMasterTester::recvTimeout() {
	sender.clearSendData();
	master->timeoutResponse();
}
