#ifndef COMMON_CCICSI_TESTPACKETLAYER_H_
#define COMMON_CCICSI_TESTPACKETLAYER_H_

#include "ccicsi/CciCsiProtocol.h"

namespace CciCsi {

class TestPacketLayer : public PacketLayerInterface {
public:
	TestPacketLayer();
	void clearSendData();
	const char *getSendData();
	void recvPacket(const char *hex);
	void recvControl(uint8_t control);
	void recvError(PacketLayerObserver::Error error);

	void setObserver(PacketLayerObserver *observer) override;
	void reset() override;
	bool sendPacket(Buffer *data) override;
	bool sendControl(uint8_t control) override;

private:
	CciCsi::PacketLayerObserver *observer;
	StringBuilder sendBuf;
	Buffer recvBuf;
};

}

#endif
