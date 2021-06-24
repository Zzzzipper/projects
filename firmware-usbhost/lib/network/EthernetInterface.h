#ifndef NETWORK_ETHERNETINTERFACE_H
#define NETWORK_ETHERNETINTERFACE_H

#include "stdint.h"

class ENC28J60;

class EthernetReceiver {
public:
	virtual ~EthernetReceiver() {}
	virtual void procRecvFrame(const uint8_t *buf, uint16_t plen) = 0;
};

class EthernetInterface {
public:
	EthernetInterface(uint8_t *mac);
	~EthernetInterface();
	void setReceiver(EthernetReceiver *receiver);
	void init();
	void shutdown();
	uint8_t *getAddr();
	uint16_t getAddrLen();
	void sendSpi(const uint8_t *data, uint16_t dataLen);
	void execute();

private:
	uint8_t mac[6];
	ENC28J60 *enc28j60;
	EthernetReceiver *receiver;
};

#endif
