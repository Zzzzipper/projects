#include "include/Hex.h"
#include "logger/include/Logger.h"

#include <string.h>

char valueHigh[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
char valueLow[]  = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

uint8_t hexToDigit(char lex) {
	for(uint8_t i = 0; i < sizeof(valueHigh); i++) {
		if(lex == valueHigh[i]) {
			return i;
		}
	}
	for(uint8_t i = 0; i < sizeof(valueLow); i++) {
		if(lex == valueLow[i]) {
			return i;
		}
	}
	return 255;
}

uint16_t hexToData(const char *hex, Buffer *buf) {
	uint16_t len = hexToData(hex, strlen(hex), buf->getData(), buf->getSize());
	buf->setLen(len);
	return len;
}

uint16_t hexToData(const char *hex, uint16_t hexLen, uint8_t *buf, uint16_t bufSize) {
	uint16_t bufLen = 0;
	for(uint16_t i = 0; i < hexLen; i++) {
		char h = hex[i];
		if(h == 'x') {
			i++;
			if(i >= hexLen) {
				LOG_ERROR(LOG_UTIL, "Wrong len " << i << ">" << hexLen);
				return 0;
			}
			h = hex[i];
		}
		uint8_t d = hexToDigit(hex[i]);
		if(d > 15) {
			LOG_ERROR(LOG_UTIL, "Wrong lex [" << i << "]=" << d);
			return 0;
		}
		buf[bufLen] = (d << 4);

		i++;
		if(i >= hexLen) {
			LOG_ERROR(LOG_UTIL, "Wrong len " << i << ">" << hexLen);
			return 0;
		}
		d = hexToDigit(hex[i]);
		if(d > 15) {
			LOG_ERROR(LOG_UTIL, "Wrong lex [" << i << "]=" << d);
			return 0;
		}
		buf[bufLen] |= d;

		bufLen++;
		if(bufLen > bufSize) {
			LOG_ERROR(LOG_UTIL, "Buffer len to small " << bufLen << ">" << bufSize);
			return 0;
		}
	}
	return bufLen;
}

char digitToHex(uint8_t digit) {
	if(digit >= sizeof(valueHigh)) { return 'X'; }
	return valueHigh[digit];
}

bool dataToHex(const uint8_t *data, uint16_t dataLen, StringBuilder *str) {
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t d = data[i];
		*str << valueHigh[d >> 4] << valueHigh[d & 0x0F];
	}
	return true;
}

uint16_t dataToHex(const uint8_t *data, uint16_t dataLen, char *buf, uint16_t bufSize) {
	uint16_t bufLen = 0;
	for(uint16_t i = 0; i < dataLen; i++) {
		if((bufLen + 2) > bufSize) {
			return bufLen;
		}
		uint8_t b = data[i];
		buf[bufLen] = valueHigh[b/16]; bufLen++;
		buf[bufLen] = valueHigh[b%16]; bufLen++;
	}
	return bufLen;
}

uint32_t hexToNumber(const char *hex, uint16_t hexLen) {
	uint32_t value = 0;
	for(uint16_t i = 0; i < hexLen; i++) {
		uint8_t d = hexToDigit(hex[i]);
		if(d > 15) { return 0; }
		value = value * 16 + d;
	}
	return value;
}
