#ifndef COMMON_UART_TESTUART_H
#define COMMON_UART_TESTUART_H

#include "uart/include/interface.h"
#include "utils/include/Fifo.h"
#include "utils/include/Buffer.h"

class TestUart : public AbstractUart {
public:
	TestUart(uint16_t size);
	virtual ~TestUart() {}

	bool addRecvData(const char *hex);
	void addRecvData(void *data, uint16_t dataLen);
	void addRecvData(uint8_t byte);
	void addRecvString(const char *str);
	uint8_t *getSendData();
	uint16_t getSendLen();
	void clearSendBuffer();

	virtual void send(uint8_t b);
	virtual void sendAsync(uint8_t b);
	virtual uint8_t receive();
	virtual bool isEmptyReceiveBuffer();
	virtual bool isFullTransmitBuffer();
	virtual void setReceiveHandler(UartReceiveHandler *handler);
	virtual void setTransmitHandler(UartTransmitHandler *handler);
	virtual void execute();

private:
	Fifo<uint8_t> recvQueue;
	Buffer sendBuffer;
	UartReceiveHandler *recieveHandler;
	UartTransmitHandler *transmitHandler;
};

#endif
