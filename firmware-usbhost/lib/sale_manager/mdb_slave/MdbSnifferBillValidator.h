#ifndef LIB_SALEMANAGER_MDBSLAVE_SNIFFERBILLVALIDATOR_H_
#define LIB_SALEMANAGER_MDBSLAVE_SNIFFERBILLVALIDATOR_H_

#include "MdbSniffer.h"

#include "common/mdb/master/bill_validator/MdbBill.h"
#include "common/mdb/slave/MdbSlavePacketReceiver.h"
#include "common/uart/include/interface.h"
#include "common/event/include/EventEngine.h"

class MdbSnifferBillValidator : public MdbSniffer, public MdbSlave::PacketObserver {
public:
	enum EventType {
		Event_Reset			 = GlobalId_SnifferBillValidator | 0x01,
		Event_Enable		 = GlobalId_SnifferBillValidator | 0x02,
		Event_Disable		 = GlobalId_SnifferBillValidator | 0x03,
		Event_DepositeBill	 = GlobalId_SnifferBillValidator | 0x04, // Mdb::EventDeposite
	};

	MdbSnifferBillValidator(MdbBillValidatorContext *context, EventEngineInterface *eventEngine);
	~MdbSnifferBillValidator() override;

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
		State_BillType,
		State_Stacker,
		State_Poll,
		State_NotPoll,
	};

	MdbSlavePacketReceiver *packetLayer;
	EventDeviceId deviceId;
	MdbBillValidatorContext *context;
	State state;
	bool enabled;

	void stateSaleCommand(const uint16_t commandId, const uint8_t *data);
	void stateSaleCommandPoll();
	void stateSaleCommandSetup();
	void stateSaleCommandExpansionIdentification();
	void stateSaleCommandBillType(const uint8_t *data);
	void stateSaleCommandStacker();

	void stateSetupCommand(const uint16_t commandId, const uint8_t *data);
	void stateSetupResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateExpansionIdentificationResponse(const uint8_t *data, uint16_t len, bool crc);
	void stateStackerResponse(const uint8_t *data, uint16_t len, bool crc);

	void statePollResponse(const uint8_t *data, uint16_t len, bool crc);
	void statePollResponseStatusDeposite(uint8_t status);
	void stateNotPollResponse(const uint8_t *data, uint16_t len, bool crc);
};

#endif
