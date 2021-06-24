#ifndef MDB_MASTER_SENDER_H_
#define MDB_MASTER_SENDER_H_

#include "mdb/MdbProtocol.h"
#include "utils/include/Buffer.h"
#include "uart/include/interface.h"

class MdbMasterSender {
public:
	MdbMasterSender();
	void setUart(AbstractUart *uart);
	void sendRequest(void *data, uint16_t dataLen);
	virtual void sendConfirm(Mdb::Control control);
	void startRequest();
	void addUint8(uint8_t data);
	void addUint16(uint16_t data);
	void addData(const uint8_t *data, uint16_t dataLen);
	virtual void sendRequest();
	uint8_t *getData() { return buf.getData(); }
	uint32_t getDataLen() { return buf.getLen(); }

protected:
	AbstractUart *uart;
	Buffer buf;
	bool command;
};

#endif
