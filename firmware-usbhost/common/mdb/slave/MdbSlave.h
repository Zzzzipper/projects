#ifndef MDB_SLAVE_H_
#define MDB_SLAVE_H_

#include "mdb/MdbProtocol.h"
#include "event/include/EventEngine.h"
#include "utils/include/Event.h"
#include "utils/include/Buffer.h"
#include "platform/include/platform.h"

//+++
//todo: разбить на три класса: MdbDevice (getType,setObserver,reset), MdbSlave(setSlaveSender,setSlaveReceiver,recvRequest,recvRequestData), MdbMaster(setMasterSender,recvResponse)
class MdbSlave {
public:
	class Sender {
	public:
		Sender();
		virtual ~Sender() {}
		virtual void sendAnswer(Mdb::Control answer) = 0;
		void sendData(const uint8_t *data, uint16_t dataLen);
		void startData();
		void addUint8(uint8_t data);
		void addUint16(uint16_t data);
		void addData(const uint8_t *data, uint16_t dataLen);
		virtual void sendData() = 0;
		uint8_t *getData() { return buf.getData(); }
		uint32_t getDataLen() { return buf.getLen(); }

	protected:
		Buffer buf;
	};

	class Receiver {
	public:
		virtual ~Receiver() {}
		virtual void recvAddress() = 0;
		virtual void recvSubcommand() = 0;
		virtual void recvRequest(uint16_t len) = 0;
		virtual void recvConfirm() = 0;
	};

	class PacketObserver {
	public:
		virtual void recvRequestPacket(const uint16_t packetType, const uint8_t *data, uint16_t dateLen) = 0;
		virtual void recvUnsupportedPacket(const uint16_t packetType) = 0;
	};

	MdbSlave(Mdb::Device type, EventEngineInterface *eventEngine) : type(type), eventEngine(eventEngine) {}
	virtual ~MdbSlave() {}
	Mdb::Device getType() { return type; }

	virtual void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) = 0;
	virtual void recvCommand(const uint8_t command) = 0;
	virtual void recvSubcommand(const uint8_t subcommand) = 0;
	virtual void recvRequest(const uint8_t *data, uint16_t len) = 0;
	virtual void recvConfirm(uint8_t control) = 0;

protected:
	void deliverEvent(EventInterface *event) { eventEngine->transmit(event); }

private:
	Mdb::Device type;
	EventEngineInterface *eventEngine;
};

#endif
