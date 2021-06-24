#ifndef LIB_SALEMANAGER_MDBSLAVE_SNIFFERCOINCHANGER_H_
#define LIB_SALEMANAGER_MDBSLAVE_SNIFFERCOINCHANGER_H_

#include "MdbSniffer.h"

#include "common/mdb/master/coin_changer/MdbCoin.h"
#include "common/mdb/slave/MdbSlavePacketReceiver.h"
#include "common/uart/include/interface.h"
#include "common/event/include/EventEngine.h"

class MdbSnifferCoinChanger : public MdbSniffer, public MdbSlave::PacketObserver {
public:
	enum EventType {
		Event_Reset			 = GlobalId_SnifferCoinChanger | 0x01,
		Event_Enable		 = GlobalId_SnifferCoinChanger | 0x02,
		Event_Disable		 = GlobalId_SnifferCoinChanger | 0x03,
		Event_TubeStatus	 = GlobalId_SnifferCoinChanger | 0x04,
		Event_DispenseCoin	 = GlobalId_SnifferCoinChanger | 0x05,
		Event_DepositeCoin	 = GlobalId_SnifferCoinChanger | 0x06, // Mdb::EventDeposite
	};

	MdbSnifferCoinChanger(MdbCoinChangerContext *context, EventEngineInterface *eventEngine);
	~MdbSnifferCoinChanger() override;

	virtual void reset() override;
	virtual bool isEnable() override;

	virtual void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) override;
	virtual void recvCommand(const uint8_t command) override;
	virtual void recvSubcommand(const uint8_t subcommand) override;
	virtual void recvRequest(const uint8_t *data, uint16_t len) override;
	virtual void recvConfirm(uint8_t control) override;

	virtual void recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) override;
	virtual void recvUnsupportedPacket(const uint16_t packetType) override { (void)packetType; }

	virtual void procResponse(const uint8_t *data, uint16_t len, bool crc) override;

private:
	enum State {
		State_Idle = 0,
		State_Sale,
		State_Setup,
		State_ExpansionIdentification,
		State_TubeStatus,
		State_Poll,
		State_NotPoll,
	};

	MdbSlavePacketReceiver *packetLayer;
	EventDeviceId deviceId;
	MdbCoinChangerContext *context;
	State state;
	bool enabled;

	void stateSaleCommand(const uint16_t commandId, const uint8_t *data);
	void stateSaleCommandPoll();
	void statePollRecvDeposite(uint8_t b1, uint8_t b2);

	void stateSaleCommandSetup();
	void stateSaleExpansionIdentification();	
	void stateSaleCommandTubeStatus();
	void stateSaleCommandExpansionPayout(const uint8_t *data);
	void stateSaleCommandCoinType(const uint8_t *data);
	void stateSaleCommandDispense(const uint8_t *data);

	void stateSetupCommand(const uint16_t commandId, const uint8_t *data);
	void stateSetupResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateExpansionIdentificationResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateTubeStatusResponse(const uint8_t *data, uint16_t len, bool crc);

	void statePollResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateNotPollResponse(const uint8_t *data, uint16_t len, bool crc);
};

#endif
