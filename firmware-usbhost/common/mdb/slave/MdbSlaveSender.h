#ifndef MDB_SLAVE_SENDER_H_
#define MDB_SLAVE_SENDER_H_

#include "mdb/slave/MdbSlave.h"
#include "uart/include/interface.h"

class MdbSlaveSender : public MdbSlave::Sender {
public:
	MdbSlaveSender();
	void setUart(AbstractUart *uart);
	virtual void sendAnswer(Mdb::Control answer);
	virtual void sendData();

protected:
	AbstractUart *uart;
};

#endif
