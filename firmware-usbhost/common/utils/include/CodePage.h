#ifndef COMMON_UTILS_CODEPAGE_H
#define COMMON_UTILS_CODEPAGE_H

#include "utils/include/StringBuilder.h"

#include "stdint.h"

extern void convertCp866ToWin1251(uint8_t *str, uint16_t strLen);
extern void convertWin1251ToCp866(uint8_t *str, uint16_t strLen);

extern uint16_t convertWin1251ToUtf8(const uint8_t *src, uint16_t srcLen, uint8_t *dst, uint16_t dstSize);
extern uint16_t convertUtf8ToWin1251(const uint8_t *src, uint16_t srcLen, uint8_t *dst, uint16_t dstSize);
extern void convertUtf8ToWin1251(const uint8_t *src, uint16_t srcLen, StringBuilder *dst);

extern char convertUnicodeToWin1251(const char *src, uint16_t srcLen);
extern void convertWin1251ToJsonUnicode(const char *src, StringBuilder *dst);
extern void convertJsonUnicodeToWin1251(const char *src, uint16_t srcLen, StringBuilder *dst);

extern uint16_t convertBinToBase64(const uint8_t *bin, uint16_t binLen, uint8_t *dst, uint16_t dstSize);
extern uint16_t convertBinToBase64(const uint8_t *bin, uint16_t binLen, StringBuilder *dst);
extern uint16_t convertBase64ToBin(const uint8_t *src, uint16_t srcLen, uint8_t *dst, uint16_t dstSize);

#endif
