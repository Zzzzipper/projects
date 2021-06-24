#ifndef COMMON_MDB_SLAVE_CASHLESS3_H_
#define COMMON_MDB_SLAVE_CASHLESS3_H_

#include "mdb/slave/MdbSlave.h"
#include "mdb/slave/MdbSlavePacketReceiver.h"
#include "timer/include/TimerEngine.h"
#include "config/include/StatStorage.h"

class MdbSlaveCashlessInterface {
public:
	enum EventType {
		Event_Reset			 = GlobalId_SlaveCashless1 | 0x01,
		Event_Enable		 = GlobalId_SlaveCashless1 | 0x02,
		Event_Disable		 = GlobalId_SlaveCashless1 | 0x03,
		Event_VendRequest	 = GlobalId_SlaveCashless1 | 0x04, // EventVendRequest
		Event_VendComplete	 = GlobalId_SlaveCashless1 | 0x05,
		Event_VendCancel	 = GlobalId_SlaveCashless1 | 0x06,
		Event_Error			 = GlobalId_SlaveCashless1 | 0x07, // Mdb::EventError
		Event_CashSale		 = GlobalId_SlaveCashless1 | 0x08, // EventVendRequest
		Event_SessionClosed	 = GlobalId_SlaveCashless1 | 0x09,
	};

	class EventVendRequest : public EventInterface {
	public:
		EventVendRequest(uint16_t type) : EventInterface(type) {}
		EventVendRequest(EventDeviceId deviceId, uint16_t type, uint16_t productId, uint32_t price) : EventInterface(deviceId, type), productId(productId), price(price) {}
		uint16_t getProductId() { return productId; }
		uint32_t getPrice() { return price; }
		virtual bool open(EventEnvelope *envelope);
		virtual bool pack(EventEnvelope *envelope);
	private:
		uint16_t productId;
		uint32_t price;
	};

	virtual ~MdbSlaveCashlessInterface() {}
	virtual void reset() = 0;
	virtual bool isInited() = 0;
	virtual bool isEnable() = 0;
	virtual void setCredit(uint32_t credit) = 0;
	virtual void approveVend(uint32_t productPrice) = 0;
	virtual void denyVend(bool close) = 0;
	virtual void cancelVend() = 0;
};

class MdbSlaveCashless3 : public MdbSlave, public MdbSlave::PacketObserver, public MdbSlaveCashlessInterface {
public:
	MdbSlaveCashless3(Mdb::Device deviceType, uint8_t maxLevel, Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine, StatStorage *stat);
	~MdbSlaveCashless3();
	EventDeviceId getDeviceId();

	void reset() override;
	bool isInited() override;
	bool isEnable() override;
	void setCredit(uint32_t credit) override;
	void approveVend(uint32_t productPrice) override;
	void denyVend(bool close) override;
	void cancelVend() override;

	virtual void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver);
	virtual void recvCommand(const uint8_t command);
	virtual void recvSubcommand(const uint8_t subcommand);
	virtual void recvRequest(const uint8_t *data, uint16_t len);
	virtual void recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	virtual void recvUnsupportedPacket(const uint16_t packetType);
	virtual void recvConfirm(uint8_t control);

	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Reset,
		State_Disabled,
		State_Enabled,
		State_SessionIdle,
		State_VendApproving,
		State_Vending,
		State_SessionCancel,
		State_SessionEnd,
		State_ReaderCancel,
	};

	enum Command {
		Command_None = 0,
		Command_SetCredit,
		Command_ApproveVend,
		Command_DenyVend,
		Command_DenyVendAndClose,
	};

	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	TimerEngine *timerEngine;
	Timer *timer;
	StatNode *state;
	StatNode *countPoll;
	uint8_t maxLevel;
	uint8_t level;
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

	void procCommandSetupConfig(const uint8_t *data);
	void procCommandSetupPricesL1(const uint8_t *data);
	void procCommandExpansionRequestId(const uint8_t *data);
	void procCommandExpansionEnableOptions(const uint8_t *data);
	void procCommandExpansionDiagnostics();
	void procCommandVendCashSale(const uint8_t *data);

	void stateResetCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void procCommandReset();
	void stateResetCommandPoll();
	void procCommandPoll();
	void procCommandPollConfirm(uint8_t control);

	void stateDisabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateDisabledReaderEnable();
	void stateDisabledCommandPoll();
	void procCommandReaderEnable();
	void procCommandReaderDisable();
	void procCommandReaderCancel();

	void gotoStateEnabled();
	void stateEnabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateEnabledCommandReaderDisable();
	void stateEnabledCommandPoll();
	void procCommandPollSessionBegin();

	void stateSessionIdleCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateSessionIdleCommandReset();
	void stateSessionIdleCommandSessionComplete();
	void stateSessionIdleCommandRevalueLimitRequest(const uint8_t *data);
	void stateSessionIdleCommandRevalueRequest(const uint8_t *);
	void stateSessionIdleCommandVendRequest(const uint8_t *data);
	void stateSessionIdleCommandPoll();
	void procCommandSessionComplete();
	void procCommandSessionCompleteConfirm(uint8_t control);

	void stateVendApprovingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateVendApprovingCommandVendRequest(const uint8_t *data);
	void stateVendApprovingCommandVendCancel();
	void stateVendApprovingCommandPoll();

	void gotoStateVending();
	void stateVendingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateVendingCommandReset();
	void stateVendingCommandSessionComplete();
	void stateVendingCommandVendSuccess(const uint8_t *data);
	void stateVendingCommandVendFailure();
	void stateVendingTimeout();

	void stateSessionCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateSessionCancelCommandPoll();
	void stateSessionCancelRevalueLimitRequest();
	void stateSessionCancelVendRequest();
	void stateSessionCancelVendSuccess();

	void stateSessionEndCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateSessionEndCommandPoll();

	void stateReaderCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateReaderCancelCommandPoll();

	void procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen);
};

#endif
