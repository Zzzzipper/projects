#ifndef COMMON_MDB_BRIDGE_H
#define COMMON_MDB_BRIDGE_H

#if 1
#include "uart/include/interface.h"
#include "utils/include/StringBuilder.h"

class MdbBridge {
public:
	MdbBridge(AbstractUart *masterUart, AbstractUart *slaveUart);

private:
	class SlaveReceiveHandler : public UartReceiveHandler {
	public:
		SlaveReceiveHandler(AbstractUart *recvUart, AbstractUart *sendUart);
		void enable(bool enabled) { this->enabled = enabled; }
		virtual void handle();

	private:
		AbstractUart *recvUart;
		AbstractUart *sendUart;
		bool enabled;
	};

	class MasterReceiveHandler : public UartReceiveHandler {
	public:
		MasterReceiveHandler(AbstractUart *recvUart, AbstractUart *sendUart, uint8_t deviceType, SlaveReceiveHandler *slaveHandler);
		virtual void handle();

	private:
		AbstractUart *recvUart;
		AbstractUart *sendUart;
		uint8_t deviceType;
		SlaveReceiveHandler *slaveHandler;
		bool enabled;
	};

	MasterReceiveHandler *masterHandler;
	SlaveReceiveHandler *slaveHandler;
};
#elif 0
#include "uart/include/interface.h"
#include "utils/include/StringBuilder.h"

class MdbBridge {
public:
	MdbBridge(AbstractUart *masterUart, AbstractUart *slaveUart);

private:
	class ReceiveHandler : public UartReceiveHandler {
	public:
		ReceiveHandler(bool master, AbstractUart *recvUart, AbstractUart *sendUart);
		virtual void handle();

	private:
		bool master;
		AbstractUart *recvUart;
		AbstractUart *sendUart;
	};

	ReceiveHandler *masterHandler;
	ReceiveHandler *slaveHandler;
};
#else
#include "common/uart/include/interface.h"
#include "common/mdb/slave/MdbSlaveReceiver.h"
#include "common/mdb/slave/MdbSlavePacketReceiver.h"
#include "common/mdb/master/MdbMasterReceiver.h"

class MdbCoinChangerSniffer : public MdbSlave, public MdbSlave::PacketObserver, public MdbMasterReceiver::Customer {
public:
	MdbCoinChangerSniffer();
	virtual void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver);
	virtual void reset();
	virtual void recvCommand(const uint8_t command);
	virtual void recvSubcommand(const uint8_t subcommand);
	virtual void recvRequest(const uint8_t *data, uint16_t len);
	virtual void recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t len);
	virtual void recvConfirm(uint8_t control);
	virtual void procResponse(const uint8_t *data, uint16_t len, bool crc);

private:
	MdbSlave::Receiver *slaveReceiver;
	MdbSlavePacketReceiver *packetLayer;
};

class MdbBridge : public MdbSlaveReceiver::Customer, public MdbMasterReceiver::Customer {
public:
	MdbBridge(AbstractUart *masterUart, AbstractUart *slaveUart);

	virtual void recvAddress(const uint8_t address);
	virtual void recvSubcommand(const uint8_t subcommand);
	virtual void recvRequest(const uint8_t *data, uint16_t len);
	virtual void recvConfirm(uint8_t control);
	virtual void procResponse(const uint8_t *data, uint16_t len, bool crc);

private:
	MdbSlaveReceiver slaveReceiver;
	MdbMasterReceiver masterReceiver;
	MdbCoinChangerSniffer coinChangerSniffer;
	MdbSlave *current;
};
#endif

#endif
