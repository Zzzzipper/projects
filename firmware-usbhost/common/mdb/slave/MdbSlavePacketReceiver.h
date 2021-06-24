#ifndef MDB_SLAVE_PACKETRECEIVER_H_
#define MDB_SLAVE_PACKETRECEIVER_H_

#include "utils/include/Buffer.h"
#include "mdb/slave/MdbSlave.h"

class MdbSlavePacketReceiver {
public:
	struct Packet {
		uint8_t command;
		uint16_t subcommand;
		uint8_t len;
		uint8_t expandedLen;
	};

	MdbSlavePacketReceiver(uint8_t device, MdbSlave::PacketObserver *observer, Packet *desc, uint8_t descNum);
	void init(MdbSlave::Receiver *receiver);
	void setMode(uint8_t mode);
	uint8_t getMode();
	void recvCommand(const uint8_t command);
	void recvSubcommand(const uint16_t subcommand);
	void recvRequest(const uint8_t *data, uint16_t len);

private:
	MdbSlave::PacketObserver *observer;
	Packet *desc;
	uint8_t descNum;
	MdbSlave::Receiver *receiver;
	uint8_t device;
	uint8_t mode;
	uint8_t command;
	uint8_t	descIndex;

	void waitSubcommand();
	void waitRequest(uint16_t len);
};

#endif
