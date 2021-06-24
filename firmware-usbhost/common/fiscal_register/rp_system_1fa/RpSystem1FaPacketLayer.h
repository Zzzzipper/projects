#ifndef COMMON_FISCALREGISTER_RPSYSTEM1FAPACKETLAYER_H
#define COMMON_FISCALREGISTER_RPSYSTEM1FAPACKETLAYER_H

class Buffer;

namespace RpSystem1Fa {

class FiscalRegister;

class PacketLayer {
public:
	PacketLayer() : observer(0) {}
	virtual ~PacketLayer() {}
	void setObserver(FiscalRegister *observer) { this->observer = observer; }
	virtual bool sendPacket(const Buffer *data) = 0;

protected:
	FiscalRegister *observer;
};

}

#endif
