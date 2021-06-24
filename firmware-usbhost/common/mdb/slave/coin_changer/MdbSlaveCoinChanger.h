#ifndef COMMON_MDB_SLAVE_COINCHANGER_H_
#define COMMON_MDB_SLAVE_COINCHANGER_H_

#include "mdb/slave/MdbSlave.h"
#include "mdb/slave/MdbSlavePacketReceiver.h"
#include "mdb/master/coin_changer/MdbCoin.h"
#include "config/include/StatStorage.h"

class MdbSlaveCoinChangerInterface {
public:
	virtual ~MdbSlaveCoinChangerInterface() {}
	virtual void reset() = 0;
	virtual bool isReseted() = 0;
	virtual bool isEnable() = 0;
	virtual void deposite(uint8_t b1, uint8_t b2) = 0;
	virtual void dispenseComplete() = 0;
	virtual void escrowRequest() = 0;
};

class MdbSlaveCoinChanger : public MdbSlave, public MdbSlave::PacketObserver, public MdbSlaveCoinChangerInterface {
public:
	enum Command {
		Command_None = 0,
		Command_Deposite,
		Command_EscrowRequest
	};

	enum EventType {
		Event_Enable			 = GlobalId_SlaveCoinChanger | 0x01,
		Event_Disable			 = GlobalId_SlaveCoinChanger | 0x02,
		Event_DispenseCoin		 = GlobalId_SlaveCoinChanger | 0x03,
		Event_Error				 = GlobalId_SlaveCoinChanger | 0x04, // Mdb::EventError
	};

	MdbSlaveCoinChanger(MdbCoinChangerContext *context, EventEngineInterface *eventEngine, StatStorage *stat);
	EventDeviceId getDeviceId();

	virtual void reset();
	virtual bool isReseted();
	virtual bool isEnable();
	virtual void deposite(uint8_t b1, uint8_t b2);
	virtual void dispenseComplete();
	virtual void escrowRequest();

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
		State_Dispense,
	};
	EventDeviceId deviceId;
	StatNode *state;
	StatNode *countPoll;
	StatNode *countDisable;
	StatNode *countEnable;
	uint16_t commandId;
	MdbCoinChangerContext *context;
	MdbSlave::Sender *slaveSender;
	MdbSlave::Receiver *slaveReceiver;
	MdbSlavePacketReceiver *packetLayer;
	Command command;
	uint16_t b1;
	uint16_t b2;
	Buffer pollData;

	void stateResetRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateResetCommandPoll();
	void procCommandReset();
	void procCommandPoll();
	void procCommandPollConfirm(uint8_t control);

	void stateDisabledRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateDisabledCommandPoll();
	void procCommandSetup();
	void procCommandExpansionIdentification();
	void procCommandExpansionFeatureEnable();
	void procCommandExpansionPayout();
	void procCommandExpansionPayoutStatus();
	void procCommandExpansionPayoutValuePoll();
	void procCommandExpansionDiagnostics();
	void procCommandTubeStatus();
	void procCommandCoinType(const uint8_t *data);
	void procCommandDispense(const uint8_t *data);
	void procCommandDeposite();
	void procCommandEscrowRequest();

	void stateEnabledRequestPacket(const uint16_t commandId, const uint8_t *dat, uint16_t dataLena);

	void stateDispenseRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateDispenseCommandPoll();
	void procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen);
};

#endif
