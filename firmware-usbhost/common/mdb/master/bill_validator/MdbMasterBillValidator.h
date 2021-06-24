#ifndef COMMON_MDB_MASTER_BILLVALIDATOR_H_
#define COMMON_MDB_MASTER_BILLVALIDATOR_H_

#include "mdb/master/MdbMaster.h"
#include "mdb/master/bill_validator/MdbBill.h"
#include "utils/include/Event.h"

class MdbMasterBillValidatorInterface {
public:
	virtual ~MdbMasterBillValidatorInterface() {}
	virtual void reset() = 0;
	virtual bool isInited() = 0;
	virtual void disable() = 0;
	virtual void enable() = 0;
};

class MdbMasterBillValidator : public MdbMaster, public MdbMasterBillValidatorInterface {
public:
	enum EventType {
		Event_Ready		= GlobalId_MasterBillValidator | 0x01,
		Event_Error		= GlobalId_MasterBillValidator | 0x02, // uint16_t errorCode
		Event_Deposite	= GlobalId_MasterBillValidator | 0x03, // uint32_t nominal
	};

	enum Error {
		Error_NotResponsible = 0x01,
	};

	enum CommandType {
		CommandType_None	 = 0x00
	};

	MdbMasterBillValidator(MdbBillValidatorContext *context, EventEngineInterface *eventEngine);
	EventDeviceId getDeviceId();

	virtual void reset();
	virtual bool isInited();
	virtual void disable();
	virtual void enable();

	virtual void initMaster(MdbMasterSender *sender);
	virtual void sendRequest();
	virtual void recvResponse(const uint8_t *data, uint16_t dataLen, bool crc);
	virtual void timeoutResponse();

private:
	enum State {
		State_Idle = 0,
		State_Reset,
		State_ResetWait,
		State_Setup,
		State_ExpansionIdentification,
		State_BillType,
		State_Poll,
		State_Escrow,
		State_EscrowConfirm,
		State_Disable,
		State_Enable
	};

	MdbBillValidatorContext *context;
	MdbMasterSender *sender;
	EventDeviceId deviceId;
	bool enabling;
	bool enabled;
	uint16_t tryCount;
	uint16_t repeatCount;
	CommandType command;

	void gotoStateReset();
	void sendReset();
	void recvReset(bool crc, const uint8_t *data, uint16_t dataLen);
	void gotoStateResetWait();
	void sendResetWait();
	void recvResetWait(bool crc, const uint8_t *data, uint16_t dataLen);

	void gotoStateSetup();
	void sendSetup();
	void recvSetup(bool crc, const uint8_t *data, uint16_t dataLen);
	void sendExpansionIdentification();
	void recvExpansionIdentification(bool crc, const uint8_t *data, uint16_t dataLen);
	void sendBillType();
	void recvBillType(bool crc, const uint8_t *data, uint16_t dataLen);

	void gotoStatePoll();
	void statePollSend();
	void statePollRecv(bool crc, const uint8_t *data, uint16_t dataLen);

	void gotoStateEscrow();
	void sendEscrow();
	void recvEscrow(bool crc, const uint8_t *data, uint16_t dataLen);
	void sendEscrowConfirm();
	void recvEscrowConfirm(bool crc, const uint8_t *data, uint16_t dataLen);

	void stateDisableSend();
	void stateDisableRecv(bool crc, const uint8_t *data, uint16_t dataLen);

	void stateEnableSend();
	void stateEnableRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	void sendPoll();
};

#endif
