#ifndef MDB_MASTER_RECEIVER_H_
#define MDB_MASTER_RECEIVER_H_

#include "uart/include/interface.h"
#include "utils/include/Buffer.h"
#include "config/include/StatStorage.h"

class MdbMasterReceiver : public UartReceiveHandler {
public:
	class Customer {
	public:
		virtual ~Customer() {}
		virtual void procResponse(const uint8_t *data, uint16_t len, bool crc) = 0;
	};

	MdbMasterReceiver(Customer *engine, StatStorage *stat);
	void setUart(AbstractUart *uart, AbstractUart *sendUart = 0);

	virtual bool isInterruptMode() { return true; }
	virtual void handle();

protected:
	Customer *customer;
	Buffer buf;
	AbstractUart *uart;
	AbstractUart *sendUart;
	StatNode *responseCount;
	StatNode *confirmCount;
	StatNode *crcErrorCount;
};

#endif
