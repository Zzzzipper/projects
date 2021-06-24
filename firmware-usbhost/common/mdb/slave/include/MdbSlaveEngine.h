#ifndef COMMON_MDB_SLAVE_ENGINE_H
#define COMMON_MDB_SLAVE_ENGINE_H

#include "mdb/slave/MdbSlave.h"
#include "mdb/slave/MdbSlaveSender.h"
#include "mdb/slave/MdbSlaveReceiver.h"

#include "utils/include/List.h"
#include "uart/include/interface.h"

class Uart;

class MdbSlaveEngine : public MdbSlaveReceiver::Customer {
public:
	MdbSlaveEngine();
	~MdbSlaveEngine();
	void init(AbstractUart* uart);
	void add(MdbSlave *slave);
	void reset();
	MdbSlave *getSlave(uint8_t type);

private:
	enum State {
		State_Idle = 0,
		State_Recv
	};
	State state;
	AbstractUart *uart;
	MdbSlaveSender sender;
	MdbSlaveReceiver receiver;
	List<MdbSlave> *slaves;
	MdbSlave *current;

	void recvAddress(const uint8_t address);
	void recvSubcommand(const uint8_t subcommand);
	void recvRequest(const uint8_t *data, uint16_t len);
	void recvConfirm(uint8_t control);

	friend MdbSlaveReceiver;
};

#endif
