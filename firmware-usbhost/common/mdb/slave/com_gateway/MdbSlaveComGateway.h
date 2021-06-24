#ifndef COMMON_MDB_SLAVE_COMGATEWAY_H_
#define COMMON_MDB_SLAVE_COMGATEWAY_H_

#include "mdb/slave/MdbSlave.h"
#include "mdb/slave/MdbSlavePacketReceiver.h"
#include "mdb/MdbProtocolComGateway.h"

class MdbSlaveComGatewayInterface {
public:
	virtual ~MdbSlaveComGatewayInterface() {}
	virtual void reset() = 0;
	virtual bool isInited() = 0;
	virtual bool isEnable() = 0;
};

class MdbSlaveComGateway : public MdbSlave, public MdbSlave::PacketObserver, public MdbSlaveComGatewayInterface {
public:
	enum Command {
		Command_None = 0,
		Command_SetCredit,
		Command_ApproveVend,
		Command_DenyVend,
	};

	enum EventType {
		Event_Reset				 = GlobalId_SlaveComGateway | 0x01,
		Event_Enable			 = GlobalId_SlaveComGateway | 0x02,
		Event_Disable			 = GlobalId_SlaveComGateway | 0x03,
		Event_ReportTranscation	 = GlobalId_SlaveComGateway | 0x04, // ReportTransaction
		Event_ReportEvent		 = GlobalId_SlaveComGateway | 0x05, // ReportEvent
		Event_Error				 = GlobalId_SlaveComGateway | 0x06,
	};

	class ReportTransaction : public EventInterface {
	public:
		Mdb::ComGateway::ReportTransactionData data;

		ReportTransaction() : EventInterface(Event_ReportTranscation) {}
		ReportTransaction(EventDeviceId deviceId) : EventInterface(deviceId, Event_ReportTranscation) {}
		virtual bool open(EventEnvelope *envelope);
		virtual bool pack(EventEnvelope *envelope);
	};

	class ReportEvent : public EventInterface {
	public:
		Mdb::ComGateway::ReportEventData data;

		ReportEvent() : EventInterface(Event_ReportEvent) {}
		ReportEvent(EventDeviceId deviceId) : EventInterface(deviceId, Event_ReportEvent) {}
		virtual bool open(EventEnvelope *envelope);
		virtual bool pack(EventEnvelope *envelope);
	};

	MdbSlaveComGateway(Mdb::DeviceContext *automat, EventEngineInterface *eventEngine);
	EventDeviceId getDeviceId();

	virtual void reset();
	virtual bool isInited();
	virtual bool isEnable();

	virtual void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver);
	virtual void recvCommand(const uint8_t command);
	virtual void recvSubcommand(const uint8_t subcommand);
	virtual void recvRequest(const uint8_t *data, uint16_t len);
	virtual void recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	virtual void recvUnsupportedPacket(const uint16_t packetType);
	virtual void recvConfirm(uint8_t control);

private:
	enum State {
		State_Idle = 0,
		State_Reset,
		State_Disabled,
		State_Enabled,
		State_SessionIdle,
		State_VendApproving,
		State_VendWait,
		State_SessionCancel,
		State_SessionEnd
	};

	EventDeviceId deviceId;
	Mdb::DeviceContext *automat;
	DecimalPointConverter converter;
	State state;
	uint16_t commandId;
	MdbSlave::Sender *slaveSender;
	MdbSlave::Receiver *slaveReceiver;
	MdbSlavePacketReceiver *packetLayer;
	Command command;
	uint32_t credit;
	uint32_t requestPrice;
	uint32_t productPrice;
	bool enabled;
	Buffer pollData;

	void stateResetCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateResetCommandPoll();
	void procCommandReset();
	void procCommandPoll();
	void procCommandPollConfirm(uint8_t control);

	void stateDisabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateDisabledCommandPoll();
	void procCommandSetup(const uint8_t *data);
	void procCommandExpansionIdentification();
	void procCommandExpansionFeatureEnable(const uint8_t *data);
	void procCommandControlEnabled();
	void procCommandControlDisabled();
	void procReportTransaction(const uint8_t *data);
	void procReportDtsEvent(const uint8_t *data);
	void procReportAssetId(const uint8_t *data);
	void procReportCurrencyId(const uint8_t *data);
	void procReportProductId(const uint8_t *data);

	void procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen);
};

#endif
