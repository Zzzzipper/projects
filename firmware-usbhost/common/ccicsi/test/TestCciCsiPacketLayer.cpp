#include "TestCciCsiPacketLayer.h"

#include <string.h>

namespace CciCsi {

TestPacketLayer::TestPacketLayer() : recvBuf(256) {}

void TestPacketLayer::clearSendData() {
	sendBuf.clear();
}

const char *TestPacketLayer::getSendData() {
	return sendBuf.getString();
}

void TestPacketLayer::recvPacket(const char *hex) {
	uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
	recvBuf.setLen(len);
	observer->procPacket(recvBuf.getData(), recvBuf.getLen());
}

void TestPacketLayer::recvControl(uint8_t control) {
	observer->procControl(control);
}

void TestPacketLayer::recvError(PacketLayerObserver::Error error) {
	observer->procError(error);
}

void TestPacketLayer::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
}

void TestPacketLayer::reset() {}

bool TestPacketLayer::sendPacket(Buffer *data) {
	for(uint16_t i = 0; i < data->getLen(); i++) {
		sendBuf.addHex((*data)[i]);
	}
	return true;
}

bool TestPacketLayer::sendControl(uint8_t control) {
	sendBuf.addHex(control);
	return true;
}

}
