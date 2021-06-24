#ifndef COMMON_MDB_SNIFFER_ENGINE_H
#define COMMON_MDB_SNIFFER_ENGINE_H

#include "MdbSniffer.h"

#include "mdb/slave/MdbSlave.h"
#include "mdb/slave/MdbSlaveSender.h"
#include "mdb/slave/MdbSlaveReceiver.h"
#include "mdb/master/MdbMasterReceiver.h"
#include "utils/include/List.h"
#include "uart/include/interface.h"

class MdbSnifferEngine : public MdbSlaveReceiver::Customer, public MdbMasterReceiver::Customer {
public:
	MdbSnifferEngine(StatStorage *stat);
	virtual ~MdbSnifferEngine();
	void init(AbstractUart *slaveUart, AbstractUart *masterUart);
	void addSlave(MdbSlave *slave);
	void addSniffer(MdbSniffer *sniffer);
	void reset();
	MdbSlave *getSlave(uint8_t type);
	MdbSniffer *getSniffer(uint8_t type);

private:
	enum State {
		State_Idle = 0,
		State_Recv
	};
	State state;
	MdbSlaveSender sender;
	MdbSlaveReceiver receiver;
	MdbMasterReceiver masterReceiver;
	List<MdbSlave> *slaves;
	List<MdbSniffer> *sniffers;
	MdbSlave *currentSlave;
	MdbSniffer *currentSniffer;

	virtual void recvAddress(const uint8_t address);
	virtual void recvSubcommand(const uint8_t subcommand);
	virtual void recvRequest(const uint8_t *data, uint16_t len);
	virtual void recvConfirm(uint8_t control);

	virtual void procResponse(const uint8_t *data, uint16_t len, bool crc);

	friend MdbSlaveReceiver;
	friend MdbMasterReceiver;
};

#endif
