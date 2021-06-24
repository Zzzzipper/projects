#include "SberbankPacketLayer.h"

#include "SberbankProtocol.h"
#include "BluetoothCrc.h"
#include "logger/include/Logger.h"

namespace Sberbank {

PacketLayer::PacketLayer(TimerEngine *timers, FrameLayerInterface *frameLayer) :
	sender(timers, frameLayer),
	receiver(timers, frameLayer)
{
	frameLayer->setObserver(this);
}

void PacketLayer::setObserver(PacketLayerObserver *observer) {
	sender.setObserver(observer);
	receiver.setObserver(observer);
}

void PacketLayer::reset() {
	LOG_DEBUG(LOG_ECLP, "reset");
	sender.reset();
	receiver.reset();
}

bool PacketLayer::sendPacket(Buffer *data) {
	return sender.sendPacket(data);
}

void PacketLayer::procPacket(const uint8_t *data, const uint16_t len) {
	receiver.procPacket(data, len);
}

void PacketLayer::procControl(uint8_t control) {
	sender.procControl(control);
}

void PacketLayer::procError(Error error) {
	LOG_DEBUG(LOG_ECLP, "procError " << error);

}

}
