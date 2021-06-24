#ifndef MDB_COINCHANGER_H_
#define MDB_COINCHANGER_H_

#include "utils/include/Event.h"
#include "mdb/master/MdbMaster.h"
#include "mdb/master/MdbMasterSender.h"
#include "mdb/master/coin_changer/MdbCoin.h"

class MdbMasterCoinChangerInterface {
public:
	virtual ~MdbMasterCoinChangerInterface() {}
	virtual void reset() = 0;
	virtual bool isInited() = 0;
	virtual bool hasChange() = 0;
	virtual void disable() = 0;
	virtual void enable() = 0;
	virtual void dispense(const uint32_t sum) = 0;
	virtual void dispenseCoin(uint8_t data) = 0;
	virtual void startTubeFilling() = 0;
	virtual void stopTubeFilling() = 0;
};

class MdbMasterCoinChanger : public MdbMaster, public MdbMasterCoinChangerInterface {
public:
	enum EventType {
		Event_Ready			 = GlobalId_MasterCoinChanger | 0x01,
		Event_Error			 = GlobalId_MasterCoinChanger | 0x02, // uint16_t errorCode
		Event_Deposite		 = GlobalId_MasterCoinChanger | 0x03, // EventCoin()
		Event_DepositeToken	 = GlobalId_MasterCoinChanger | 0x04, // EventCoin()
		Event_Dispense		 = GlobalId_MasterCoinChanger | 0x05, // EventCoin()
		Event_DispenseCoin	 = GlobalId_MasterCoinChanger | 0x06, // EventCoin()
		Event_DispenseManual = GlobalId_MasterCoinChanger | 0x07, // EventCoin()
		Event_EscrowRequest  = GlobalId_MasterCoinChanger | 0x08,
	};

	enum Route {
		Route_None		= 0x00,
		Route_Cashbox	= 0x01,
		Route_Tube		= 0x02,
		Route_Reject	= 0x03,
	};

	class EventCoin : public EventInterface {
	public:
		EventCoin(uint16_t type) : EventInterface(type) {}
		EventCoin(EventDeviceId deviceId) : EventInterface(deviceId, 0) {}
		EventCoin(EventDeviceId deviceId, uint16_t type, uint32_t nominal, uint8_t route, uint8_t b1, uint8_t b2);
		void set(uint16_t type, uint32_t nominal, uint8_t route, uint8_t b1, uint8_t b2);
		uint32_t getNominal() { return nominal; }
		uint8_t getRoute() { return route; }
		uint8_t getByte1() { return b1; }
		uint8_t getByte2() { return b2; }
		virtual bool open(EventEnvelope *event);
		virtual bool pack(EventEnvelope *event);
	private:
		uint32_t nominal;
		uint8_t route;
		uint8_t b1;
		uint8_t b2;
	};

	enum Error {
		Error_NotResponsible = 0x01,
	};

	enum CommandType {
		CommandType_None			 = 0x00,
		CommandType_Dispense		 = 0x01, // uint16_t value
		CommandType_DispenseCoin	 = 0x02, // uint8_t data (number & index)
		CommandType_TubeFillingStart = 0x03,
		CommandType_TubeFillingStop	 = 0x04,
	};

	MdbMasterCoinChanger(MdbCoinChangerContext *context, EventEngineInterface *eventEngine);
	EventDeviceId getDeviceId();

	virtual void reset();
	virtual bool isInited();
	virtual bool hasChange();
	virtual void dispense(const uint32_t sum);
	virtual void dispenseCoin(uint8_t data);
	virtual void startTubeFilling();
	virtual void stopTubeFilling();
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
		State_ExpansionFeatureEnable,
		State_TubeStatus,
		State_CoinType,
		State_Poll,
		State_PollTubeStatus,
		State_PollDiagnostic,
		State_Disable,
		State_Enable,
		State_ExpansionPayout,
		State_ExpansionPayoutWait,
		State_ExpansionPayoutStatus,
		State_DispenseCoin,
		State_DispenseWait,
		State_DispenseTubeStatus,
		State_TubeFillingInit,
		State_TubeFilling,
		State_TubeFillingUpdate,
	};

	MdbMasterSender *sender;
	EventDeviceId deviceId;
	bool diagnostic;
	bool enabling;
	bool enabled;
	uint16_t tryCount;
	uint16_t repeatCount;
	MdbCoinChangerContext *context;
	CommandType command;
	uint32_t dispenseStep;
	uint32_t dispenseSum;
	uint32_t dispenseCurrent;
	uint16_t dispenseCount;
	uint8_t coinData;
	EventCoin eventCoin;

	void gotoStateReset();
	void sendReset();
	void recvReset(const uint8_t *data, uint16_t dataLen, bool crc);
	void gotoStateResetWait();
	void sendResetWait();
	void recvResetWait(const uint8_t *data, uint16_t dataLen, bool crc);

	void gotoStateSetup();
	void sendSetup();
	void recvSetup(const uint8_t *data, uint16_t dataLen, bool crc);

	void sendExpansionIdentification();
	void recvExpansionIdentification(const uint8_t *data, uint16_t dataLen, bool crc);

	void sendExpansionFeatureEnable();
	void recvExpansionFeatureEnable(const uint8_t *data, uint16_t dataLen, bool crc);

	void stateTubeStatusSend();
	void stateTubeStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc);

	void sendCoinType();
	void recvCoinType(const uint8_t *data, uint16_t dataLen, bool crc);

	void gotoStatePoll();
	void statePollSend();
	void statePollRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void statePollRecvDeposite(uint8_t b1, uint8_t b2);
	void statePollRecvDispense(uint8_t b1, uint8_t b2);
	void statePollRecvEscrowRequest();
	void statePollTubeStatusSend();
	void statePollTubeStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void statePollDiagnosticSend();
	void statePollDiagnosticRecv(const uint8_t *data, uint16_t dataLen, bool crc);

	void stateDisableSend();
	void stateDisableRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void stateEnableSend();
	void stateEnableRecv(const uint8_t *data, uint16_t dataLen, bool crc);

	void gotoStateExpansionPayout();
	void stateExpansionPayoutSend();
	void stateExpansionPayoutRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void stateExpansionPayoutWaitSend();
	void stateExpansionPayoutWaitRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void stateExpansionPayoutStatusSend();
	void stateExpansionPayoutStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc);

	void stateDispenseCoinSend();
	void stateDispenseCoinRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void stateDispenseWaitSend();
	void stateDispenseWaitRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void stateDispenseTubeStatusSend();
	void stateDispenseTubeStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc);

	void stateTubeFillingInitSend();
	void stateTubeFillingInitRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void stateTubeFillingSend();
	void stateTubeFillingRecv(const uint8_t *data, uint16_t dataLen, bool crc);
	void stateTubeFillingUpdateSend();
	void stateTubeFillingUpdateRecv(const uint8_t *data, uint16_t dataLen, bool crc);

	void sendTubeStatus();
	bool recvTubeStatus(const uint8_t *data, uint16_t dataLen, bool crc);
	void sendPoll();
};

#endif /* COINCHANGER_H_ */
