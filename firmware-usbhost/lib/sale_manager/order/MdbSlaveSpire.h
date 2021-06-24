#ifndef LIB_SALEMANAGER_ORDER_MDBSLAVESPIRE_H_
#define LIB_SALEMANAGER_ORDER_MDBSLAVESPIRE_H_

#include "common/ccicsi/CciT3Driver.h"
#include "common/mdb/slave/MdbSlave.h"
#include "common/mdb/slave/MdbSlavePacketReceiver.h"
#include "common/timer/include/TimerEngine.h"
#include "common/config/include/StatStorage.h"

class MdbSlaveSpire : public MdbSlave, public MdbSlave::PacketObserver, public OrderDeviceInterface {
public:
	MdbSlaveSpire(Mdb::Device deviceType, uint8_t maxLevel, Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine, StatStorage *stat);
	~MdbSlaveSpire();
	EventDeviceId getDeviceId();

	void setOrder(Order *order) override;
	void reset() override;
	void disable() override;
	void enable() override;
	void approveVend() override;
	void requestPinCode() override;
	void denyVend() override;

	void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) override;
	void recvCommand(const uint8_t command) override;
	void recvSubcommand(const uint8_t subcommand) override;
	void recvRequest(const uint8_t *data, uint16_t len) override;
	void recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) override;
	void recvUnsupportedPacket(const uint16_t packetType) override;
	void recvConfirm(uint8_t control) override;

	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Disabled,
		State_Enabled,
		State_SessionIdle,
		State_PinCode,
		State_VendApproving,
		State_Vending,
		State_NextRequest,
		State_SessionCancel,
		State_SessionEnd,
		State_ReaderCancel,
	};

	enum Command {
		Command_None = 0,
		Command_SetCredit,
		Command_ApproveVend,
		Command_PinCode,
		Command_DenyVend,
		Command_DenyVendAndClose,
	};

	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	TimerEngine *timerEngine;
	Timer *timer;
	StatNode *state;
	StatNode *countPoll;
	Order *order;
	uint16_t orderCid;
	uint8_t maxLevel;
	uint8_t level;
	uint16_t commandId;
	MdbSlave::Sender *slaveSender;
	MdbSlave::Receiver *slaveReceiver;
	MdbSlavePacketReceiver *packetLayer;
	Command command;
	uint32_t credit;
	bool session;
	bool enabled;
	bool first;
	Buffer pollData;

	void procCommandSetupConfig(const uint8_t *data);
	void procCommandSetupPricesL1(const uint8_t *data);
	void procCommandExpansionRequestId(const uint8_t *data);
	void procCommandExpansionEnableOptions(const uint8_t *data);
	void procCommandExpansionDiagnostics();
	void procCommandVendCashSale(const uint8_t *data);
	void procCommandReset();
	void procCommandPoll();
	void procCommandPollConfirm(uint8_t control);
	void procCommandRevalueLimitRequest();

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
	void stateSessionIdleCommandOrderRequest(const uint8_t *data);
	void stateSessionIdleCommandPoll();
	void procCommandSessionComplete();
	void procCommandSessionCompleteConfirm(uint8_t control);

	void gotoStatePinCode();
	void statePinCodeCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void statePinCodeCommandReset();
	void statePinCodeCommandSessionComplete();
	void statePinCodeCommandOrderRequest(const uint8_t *data);
	void statePinCodeCommandPoll();

	void stateVendApprovingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateVendApprovingCommandOrderRequest(const uint8_t *data);
	void stateVendApprovingCommandVendCancel();
	void stateVendApprovingCommandPoll();

	void gotoStateVending();
	void stateVendingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateVendingCommandReset();
	void stateVendingCommandSessionComplete();
	void stateVendingCommandVendSuccess(const uint8_t *data);
	void stateVendingCommandVendFailure();
	void stateVendingTimeout();

	void stateNextRequestCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateNextRequestCommandSessionComplete();
	void stateNextRequestCommandOrderRequest(const uint8_t *data);

	void stateSessionCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateSessionCancelCommandPoll();
	void stateSessionCancelVendRequest();
	void stateSessionCancelVendSuccess();

	void stateSessionEndCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateSessionEndCommandPoll();

	void stateReaderCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateReaderCancelCommandPoll();

	void procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen);
};

#endif
