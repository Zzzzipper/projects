#ifndef COMMON_DEX_RECEIVER_H_
#define COMMON_DEX_RECEIVER_H_

#include "utils/include/Buffer.h"
#include "dex/DexProtocol.h"
#include "uart/include/interface.h"

namespace Dex {

class Server;

class Receiver : public UartReceiveHandler {
public:
	class Customer {
	public:
		virtual ~Customer() {}
		virtual void procControl(DexControl control) = 0;
		virtual void procData(const uint8_t *data, const uint16_t len, bool last) = 0;
		virtual void procConfirm(const uint8_t number) = 0;
		virtual void procWrongCrc() = 0;
	};

	Receiver(Customer *customer);
	virtual ~Receiver();
	void setConnection(AbstractUart *uart);
	void reset();
	void handle();

protected:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_DLE,
		State_Data,
		State_Flag,
		State_Crc0,
		State_Crc1
	};
	Customer *customer;
	AbstractUart *uart;
	State state;
	Buffer buf;
	uint8_t flag;
	uint8_t crc0;
	uint8_t crc1;

	bool checkDataCrc();
};

}
#endif
