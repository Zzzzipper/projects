#ifndef COMMON_MDB_SLAVE_BILLVALIDATOR_H_
#define COMMON_MDB_SLAVE_BILLVALIDATOR_H_

#include "mdb/slave/MdbSlave.h"
#include "mdb/slave/MdbSlavePacketReceiver.h"
#include "mdb/master/bill_validator/MdbBill.h"

class MdbSlaveBillValidatorInterface {
public:
	virtual ~MdbSlaveBillValidatorInterface() {}
	virtual void reset() = 0;
	virtual bool isReseted() = 0;
	virtual bool isEnable() = 0;
	virtual bool deposite(uint8_t index) = 0;
};

class MdbSlaveBillValidator : public MdbSlave, public MdbSlave::PacketObserver, public MdbSlaveBillValidatorInterface {
public:
	enum Command {
		Command_None = 0,
		Command_Deposite,
	};

	enum EventType {
		Event_Error				 = GlobalId_SlaveBillValidator | 0x01, // Mdb::EventError
		Event_Enable			 = GlobalId_SlaveBillValidator | 0x02,
		Event_Disable			 = GlobalId_SlaveBillValidator | 0x03
	};

	MdbSlaveBillValidator(MdbBillValidatorContext *context, EventEngineInterface *eventEngine);
	void reset() override;
	bool isReseted() override;
	bool isEnable() override;
	bool deposite(uint8_t index) override;

	void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) override;
	void recvCommand(const uint8_t command) override;
	void recvSubcommand(const uint8_t subcommand) override;
	void recvRequest(const uint8_t *data, uint16_t len) override;
	void recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) override;
	void recvUnsupportedPacket(const uint16_t packetType) override;
	void recvConfirm(uint8_t control) override;

private:
	enum State {
		State_Idle = 0,
		State_Reset,
		State_Disabled,
		State_Enabled
	};
	EventDeviceId deviceId;
	State state;
	Command command;
	uint8_t b1;
	uint16_t commandId;
	MdbBillValidatorContext *context;
	MdbSlave::Sender *slaveSender;
	MdbSlave::Receiver *slaveReceiver;
	MdbSlavePacketReceiver *packetLayer;
	Buffer pollData;

	void stateResetRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void stateResetCommandPoll();
	void procCommandReset();
	void procCommandPoll2();
	void procCommandDeposite();
	void procCommandPoll();
	void procCommandPollConfirm(uint8_t control);
	void stateDisabledRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);
	void procCommandSetup();
	void procCommandSecurity();
	void procCommandExpansionIdentificationL1();
	void procCommandEscrow();
	void procCommandStacker();
	void procCommandBillType(const uint8_t *data);

	void stateEnabledRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen);

	void procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen);
};

#endif
