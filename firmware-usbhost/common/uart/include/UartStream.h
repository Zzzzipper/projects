#ifndef COMMON_UART_UARTSTREAM_H_
#define COMMON_UART_UARTSTREAM_H_

#include "uart/include/interface.h"
#include "utils/include/StringBuilder.h"

class UartStream {
public:
	UartStream(AbstractUart *uart) : uart(uart) {}
	void send(const uint8_t *p, uint16_t len);
	void send(StringBuilder &str);
	void send(const char *str);
	void sendln(const char *str);

private:
	AbstractUart *uart;
};

#endif
