#ifndef COMMON_UTILS_HEX_H_
#define COMMON_UTILS_HEX_H_

#include "Buffer.h"
#include "StringBuilder.h"

#include <stdint.h> //uint8_t

extern uint8_t hexToDigit(char lex);
extern uint16_t hexToData(const char *hex, Buffer *buf);
extern uint16_t hexToData(const char *hex, uint16_t hexLen, uint8_t *buf, uint16_t bufSize);
extern char digitToHex(uint8_t digit);
extern bool dataToHex(const uint8_t *data, uint16_t dataLen, StringBuilder *str);
extern uint16_t dataToHex(const uint8_t *data, uint16_t dataLen, char *buf, uint16_t bufSize);
extern uint32_t hexToNumber(const char *hex, uint16_t hexLen);

#endif
