#ifndef LIB_SALEMANAGER_MDBSLAVE_SNIFFERCOMMGATEWAY_H_
#define LIB_SALEMANAGER_MDBSLAVE_SNIFFERCOMMGATEWAY_H_

#include "MdbSniffer.h"

#include "common/mdb/slave/MdbSlavePacketReceiver.h"
#include "common/uart/include/interface.h"
#include "common/event/include/EventEngine.h"

class MdbSnifferComGateway : public MdbSniffer, public MdbSlave::PacketObserver {
public:
	MdbSnifferComGateway(Mdb::Device deviceType, Mdb::DeviceContext *context, EventEngineInterface *eventEngine);
	~MdbSnifferComGateway();

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

#if 0
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
#endif
};

#endif
