#include "include/LogTargetUart.h"
#include "uart/include/interface.h"

LogTargetUart::LogTargetUart(AbstractUart *uart) : uart(uart) {}

void LogTargetUart::send(const uint8_t *data, const uint16_t len) {
	for(uint16_t i = 0; i < len; i++) {
		uart->sendAsync(data[i]);
	}
}
