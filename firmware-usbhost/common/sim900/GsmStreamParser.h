#ifndef COMMON_GSM_STREAMPARSER_H_
#define COMMON_GSM_STREAMPARSER_H_

#include "uart/include/interface.h"
#include "utils/include/Buffer.h"

namespace Gsm {

class StreamParser : public UartReceiveHandler {
public:
	class Customer {
	public:
		virtual ~Customer() {}
		virtual void procLine(const char *line, uint16_t lineLen) = 0;
		virtual bool procData(uint8_t) = 0;
	};

	StreamParser(AbstractUart *uart);
	void reset();
	void sendLine(const char *data);
	void sendData(const uint8_t *data, uint16_t dataLen);
	void readLine(Customer *customer);
	void waitSymbol(uint8_t symbol, Customer *customer);
	void readData(Customer *customer);
	virtual void handle();
	void procTimeout();

private:
	enum State {
		State_Idle = 0,
		State_Line,
		State_WaitSymbol,
		State_Data,
	};

	AbstractUart *uart;
	State state;
	bool crFlag;
	uint8_t symbol;
	Buffer buf;
	Customer *customer;

	void stateLineByte(uint8_t b);
	void stateDataByte(uint8_t b);
	void stateWaitSymbolByte(uint8_t b);
	void deliverLine();
};

}
#endif
