#ifndef LIB_SALEMANAGER_MDBSLAVE_SNIFFERCASHLESS_H_
#define LIB_SALEMANAGER_MDBSLAVE_SNIFFERCASHLESS_H_

#include "MdbSniffer.h"

#include "common/mdb/slave/MdbSlavePacketReceiver.h"
#include "common/uart/include/interface.h"
#include "common/event/include/EventEngine.h"

class MdbSnifferCashless : public MdbSniffer, public MdbSlave::PacketObserver {
public:
	enum EventType {
		Event_Reset			 = GlobalId_SnifferCashless | 0x01,
		Event_Enable		 = GlobalId_SnifferCashless | 0x02,
		Event_Disable		 = GlobalId_SnifferCashless | 0x03,
		Event_VendComplete	 = GlobalId_SnifferCashless | 0x04, // EventVendRequest
	};

	class EventVend : public EventInterface {
	public:
		EventVend();
		EventVend(EventDeviceId deviceId, uint16_t productId, uint32_t price);
		uint16_t getProductId() { return productId; }
		uint32_t getPrice() { return price; }
		virtual bool open(EventEnvelope *envelope);
		virtual bool pack(EventEnvelope *envelope);
	private:
		uint16_t productId;
		uint32_t price;
	};

	MdbSnifferCashless(Mdb::Device deviceType, Mdb::DeviceContext *context, EventEngineInterface *eventEngine);
	~MdbSnifferCashless();

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
		State_SetupConfig,
		State_ExpansionIdentification,
		State_VendRequest,
		State_VendSuccess,
		State_Poll,
		State_NotPoll,
	};

	MdbSlavePacketReceiver *packetLayer;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	State state;
	bool enabled;
	uint8_t level;
	uint16_t cashlessId;
	uint32_t price;

	void stateSaleCommand(const uint16_t commandId, const uint8_t *data);
	void stateSaleCommandPoll();
	void stateSaleCommandSetupConfig();
	void stateSaleCommandExpansionIdentification();
	void stateSaleCommandReaderDisable();
	void stateSaleCommandReaderEnable();
	void stateSaleCommandVendRequest(const uint8_t *data);
	void stateSaleCommandVendSuccess(const uint8_t *data);

	void stateSetupConfigResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateExpansionIdentificationResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateVendRequestResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateVendSuccessResponse(const uint8_t *data, uint16_t len, bool crc);
	void statePollResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateNotPollResponse(const uint8_t *data, uint16_t len, bool crc);
};

#endif
