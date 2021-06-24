#ifndef MDB_SLAVE_RECEIVER_H_
#define MDB_SLAVE_RECEIVER_H_

#include "uart/include/interface.h"
#include "utils/include/Buffer.h"
#include "mdb/slave/MdbSlave.h"

class MdbSlaveReceiver : public UartReceiveHandler, public MdbSlave::Receiver {
public:
	class Customer {
	public:
		virtual ~Customer() {}
		virtual void recvAddress(const uint8_t address) = 0;
		virtual void recvSubcommand(const uint8_t subcommand) = 0;
		virtual void recvRequest(const uint8_t *data, uint16_t len) = 0;
		virtual void recvConfirm(uint8_t control) = 0;
	};

	MdbSlaveReceiver(Customer *context);
	void setUart(AbstractUart *uart, AbstractUart *sendUart = NULL);
	void recvAddress();
	void recvSubcommand();
	void recvRequest(uint16_t len);
	void recvConfirm();
	void stop();

	virtual bool isInterruptMode() { return true; }
	virtual void handle();

protected:
	enum State {
		State_Idle = 0,
		State_Address,
		State_Subcommand,
		State_Request,
		State_Confirm
	};
	Customer *context;
	AbstractUart *uart;
	AbstractUart *sendUart;
	State state;
	Buffer buf;
	uint16_t recvLen;
};

#endif
