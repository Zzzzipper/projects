#ifndef COMMON_MDB_MASTER_CASHLESS_H_
#define COMMON_MDB_MASTER_CASHLESS_H_

#include "MdbMasterCashlessInterface.h"

#include "mdb/master/MdbMaster.h"
#include "timer/include/TimerEngine.h"

#define MDB_CL_SESSION_TIMEOUT 120000
#define MDB_CL_APPROVING_TIMEOUT 120000
#define MDB_CL_VENDING_TIMEOUT 120000

class MdbMasterCashless : public MdbMaster, public MdbMasterCashlessInterface {
public:
	enum Error {
		Error_NotResponsible = 0x01,
	};

	MdbMasterCashless(Mdb::Device type, Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine);
	virtual ~MdbMasterCashless();

	EventDeviceId getDeviceId() override;
	void reset() override;
	bool isRefundAble() override;
	void disable() override;
	void enable() override;
	bool revalue(uint32_t credit) override;
	bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) override;
	bool saleComplete() override;
	bool saleFailed() override;
	bool closeSession() override;

	virtual void initMaster(MdbMasterSender *sender);
	virtual void sendRequest();
	virtual void recvResponse(const uint8_t *data, uint16_t dataLen, bool crc);
	virtual void timeoutResponse();

	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Reset,
		State_ResetWait,
		State_SetupConfig,
		State_SetupConfigWait,
		State_SetupPrices,
		State_ExpansionIdentification,
		State_ExpansionIdentificationWait,
		State_Work,
		State_Disable,
		State_Enable,
		State_Session,
		State_Revalue,
		State_RevalueWait,
		State_VendRequest,
		State_VendApproving,
		State_VendCancel,
		State_VendCancelling,
		State_Vending,
		State_VendSuccess,
		State_VendFailure,
		State_SessionComplete,
		State_SessionEnd,
	};

	enum CommandType {
		CommandType_None		 = 0x00,
		CommandType_Revalue		 = 0x01,
		CommandType_VendRequest	 = 0x02,
		CommandType_VendSuccess	 = 0x03,
		CommandType_VendFailure  = 0x04,
		CommandType_SessionClose = 0x05,
	};

	Mdb::DeviceContext *context;
	TimerEngine *timerEngine;
	Timer *timer;
	MdbMasterSender *sender;
	EventDeviceId deviceId;
	bool refundAble;
	bool enabling;
	bool enabled;
	uint16_t tryCount;
	uint16_t repeatCount;
	uint8_t featureLevel;
	CommandType command;
	bool timeout;
	uint16_t productId;
	uint32_t productPrice;

	void gotoStateReset();
	void sendReset();
	void recvReset(bool crc, const uint8_t *data, uint16_t dataLen);
	void gotoStateResetWait();
	void sendResetWait();
	void recvResetWait(bool crc, const uint8_t *data, uint16_t dataLen);

	void sendPoll();
	void gotoStateSetupConfig();
	void sendSetupConfig();
	void recvSetupConfig(bool crc, const uint8_t *data, uint16_t dataLen);
	void sendSetupPrices();
	void recvSetupPrices();
	void sendExpansionIdentification();
	void recvExpansionIdentification(bool crc, const uint8_t *data, uint16_t dataLen);

	void gotoStateWork();
	void stateWorkSend();
	void stateWorkRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	uint16_t procBeginSession(const uint8_t *data, uint16_t dataLen);

	void stateEnableSend();
	void stateEnableRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	void stateDisableSend();
	void stateDisableRecv(bool crc, const uint8_t *data, uint16_t dataLen);

	void stateSessionSend();
    void stateSessionRecv(bool crc, const uint8_t *data, uint16_t dataLen);

	void stateRevalueSend();
	void stateRevalueRecv(bool crc, const uint8_t *data, uint16_t dataLen);

	void gotoStateVendRequest();
	void stateVendRequestSend();
	void stateVendRequestRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	void stateVendApprovingSend();
	void stateVendApprovingRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	uint16_t procVendApproved(const uint8_t *data, uint16_t dataLen);
	uint16_t procVendDenied(uint16_t dataLen);
	void stateVendingSend();
	void stateVendingRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	void gotoStateVendSuccess();
	void stateVendSuccessSend();
	void stateVendSuccessRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	void gotoStateVendFailure();
	void stateVendFailureSend();
	void stateVendFailureRecv(bool crc, const uint8_t *data, uint16_t dataLen);

	void gotoStateVendCancel();
	void stateVendCancelSend();
	void stateVendCancelRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	void stateVendCancellingSend();
	void stateVendCancellingRecv(bool crc, const uint8_t *data, uint16_t dataLen);

	void gotoSessionComplete();
	void stateSessionCompleteSend();
	void stateSessionCompleteRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	void stateSessionEndSend();
	void stateSessionEndRecv(bool crc, const uint8_t *data, uint16_t dataLen);
	uint16_t procSessionEnd();
	void stateSessionEndTimeout();
};

#endif
