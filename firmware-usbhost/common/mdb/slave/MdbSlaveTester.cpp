#include "MdbSlaveTester.h"

#include "utils/include/Hex.h"
#include "logger/include/Logger.h"
#include "common.h"

#include <string.h>

void MdbSlaveTester::Sender::sendAnswer(Mdb::Control control) {
	sendType = SendType_Confirm;
	buf.addUint8(control);
}

void MdbSlaveTester::Sender::sendData() {
	sendType = SendType_Data;
}

uint8_t *MdbSlaveTester::Sender::getSendData() {
	return buf.getData();
}

uint16_t MdbSlaveTester::Sender::getSendDataLen() {
	if(sendType != SendType_Data) { return 0; }
	return buf.getLen();
}

uint8_t *MdbSlaveTester::Sender::getSendConfirm() {
	return buf.getData();
}

uint16_t MdbSlaveTester::Sender::getSendConfirmLen() {
	if(sendType != SendType_Confirm) { return 0; }
	return buf.getLen();
}

void MdbSlaveTester::Sender::clearSendData() {
	sendType = SendType_None;
	buf.clear();
}

MdbSlaveTester::Receiver::Receiver(MdbSlave *slave) : slave(slave), buf(256) {}

void MdbSlaveTester::Receiver::recvAddress() {
	LOG_ERROR(LOG_TEST, "recvAddress " << buf.getLen());
	if(buf.getLen() < 1) {
		LOG_ERROR(LOG_TEST, "Not enough data (act=" << buf.getLen() << ", exp=1+)");
		return;
	}
	uint8_t command = buf[0] & MDB_COMMAND_MASK;
	slave->recvCommand(command);
}

void MdbSlaveTester::Receiver::recvSubcommand() {
	LOG_ERROR(LOG_TEST, "recvSubcommand " << buf.getLen());
	if(buf.getLen() < 2) {
		LOG_ERROR(LOG_TEST, "Not enough data (act=" << buf.getLen() << ", exp=2+)");
		return;
	}
	slave->recvSubcommand(buf[1]);
}

void MdbSlaveTester::Receiver::recvRequest(uint16_t len) {
	LOG_ERROR(LOG_TEST, "recvRequest " << len << "," << buf.getLen());
	if(len != buf.getLen()) {
		LOG_ERROR(LOG_TEST, "Wrong data len (act=" << buf.getLen() << ", exp=" << len << ")");
		return;
	}
	slave->recvRequest(buf.getData(), len);
}

void MdbSlaveTester::Receiver::recvConfirm() {

}

bool MdbSlaveTester::Receiver::recvCommand(const char *command) {
	uint16_t len = hexToData(command, strlen(command), buf.getData(), buf.getSize());
	if(len == 0) {
		return false;
	}
	buf.setLen(len);	
	slave->recvCommand(buf[0] & MDB_COMMAND_MASK);
	return true;
}

bool MdbSlaveTester::Receiver::recvConfirm(uint8_t control) {
	slave->recvConfirm(control);
	return true;
}

MdbSlaveTester::MdbSlaveTester(MdbSlave *slave) :
	slave(slave),
	receiver(slave)
{
	slave->initSlave(&sender, &receiver);
}

bool MdbSlaveTester::recvCommand(const char *command) {
	sender.clearSendData();
	return receiver.recvCommand(command);
}

bool MdbSlaveTester::recvConfirm(uint8_t control) {
	sender.clearSendData();
	return receiver.recvConfirm(control);
}
