#include "EvadtsChecker.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

#include <string.h>

#define EVADTS_LINE_MAX_SIZE 256
#define SYMBOL_CR 0x0D
#define SYMBOL_LF 0x0A

namespace Evadts {

Checker::Checker() {
	this->str = new char[EVADTS_LINE_MAX_SIZE]();
}

Checker::~Checker() {
	delete[] this->str;
}

bool Checker::hasError() {
	return error;
}

void Checker::start() {
	LOG_DEBUG(LOG_EVADTS, "start");
	strLen = 0;
	symbolCR = false;
	crc.start();
	error = false;
}

void Checker::procData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_EVADTS, "procData");
	LOG_DEBUG_STR(LOG_EVADTS, data, len);
	if(error == true) {
		return;
	}
	for(uint16_t i = 0; i < len; i++) {
		LOG_TRACE_HEX(LOG_EVADTS, data + i, 1);
		if(data[i] == SYMBOL_CR) {
			symbolCR = true;
		} else if(data[i] == SYMBOL_LF) {
			if(symbolCR == false) {
				LOG_DEBUG(LOG_EVADTS, "LF found, CR not found");
				error = true;
				return;
			}
			if(addSymbol(SYMBOL_CR) == false) {
				LOG_DEBUG(LOG_EVADTS, "addSymbol failed");
				error = true;
				return;
			}
			if(addSymbol(SYMBOL_LF) == false) {
				LOG_DEBUG(LOG_EVADTS, "addSymbol failed");
				error = true;
				return;
			}
			if(addSymbol('\0') == false) {
				LOG_DEBUG(LOG_EVADTS, "Line termination failed");
				error = true;
				return;
			}
			if(procLine() == false) {
				error = true;
				return;
			}
			strLen = 0;
			symbolCR = false;
		} else {
			if(addSymbol(data[i]) == false) {
				LOG_DEBUG(LOG_EVADTS, "addSymbol failed");
				error = true;
				return;
			}
			symbolCR = false;
		}
	}
}

void Checker::complete() {
	LOG_DEBUG(LOG_EVADTS, "complete");

}

bool Checker::addSymbol(char symbol) {
	if(strLen >= EVADTS_LINE_MAX_SIZE) {
		LOG_ERROR(LOG_EVADTS, "Too long line");
		return false;
	}
	str[strLen] = symbol;
	strLen++;
	return true;
}

bool Checker::procLine() {
	if(strncmp(str, "DXS", 3) == 0) { return true; }
	if(strncmp(str, "G85", 3) == 0) { return procG85(); }
	if(strncmp(str, "SE", 2) == 0) {  return true; }
	if(strncmp(str, "DXE", 3) == 0) { return true; }
	else { return procOther(); }
}

bool Checker::procOther() {
	LOG_DEBUG_STR(LOG_EVADTS, str, strLen);
	crc.add(str, strLen - 1);
	return true;
}

bool Checker::procG85() {
	LOG_DEBUG_STR(LOG_EVADTS, str, strLen);
	if(strncmp(str, "G85*", 4) != 0 || strLen < 11) {
		LOG_ERROR(LOG_EVADTS, "Wrong G85");
		return false;
	}
	uint8_t crcValue[3];
	if(hexToData(str + 4, 4, crcValue, 3) != 2) {
		LOG_ERROR(LOG_EVADTS, "Wrong crc format");
		return false;
	}
	LOG_DEBUG(LOG_EVADTS, "exp=" << crcValue[0] << "," << crcValue[1] << "act=" << crc.getHighByte() << "," << crc.getLowByte());

	if(crcValue[0] != crc.getHighByte() || crcValue[1] != crc.getLowByte()) {
		LOG_ERROR(LOG_EVADTS, "Wrong crc value");
		return false;
	}

	return true;
}

}
