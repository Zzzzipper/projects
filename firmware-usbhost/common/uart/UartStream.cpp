#include "uart/include/UartStream.h"

#include <stdint.h>

void UartStream::send(const uint8_t *p, uint16_t len) {
	for(uint16_t i = 0; i < len; i++) {
		uart->send(p[i]);
	}
}

void UartStream::send(StringBuilder &str) {
	send((uint8_t *)str.getString(), str.getLen());
}

void UartStream::send(const char *str) {
	uint8_t *p = (uint8_t *)str;
	while(*p) {
		uart->send(*p++);
	}
}

void UartStream::sendln(const char *str) {
	send(str);
	send("\r\n");
}
