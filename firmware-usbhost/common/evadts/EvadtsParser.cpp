#include "evadts/EvadtsParser.h"
#include "utils/include/Number.h"
#include "utils/include/Buffer.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

#include <string.h>

#define EVADTS_LINE_MAX_SIZE 256
#define EVADTS_TOKEN_MAX_NUM 32
#define SYMBOL_CR 0x0D
#define SYMBOL_LF 0x0A

namespace Evadts {

Parser::Parser() {
	this->str = new char[EVADTS_LINE_MAX_SIZE]();
	this->tokens = new char*[EVADTS_TOKEN_MAX_NUM]();
}

Parser::~Parser() {
	delete[] this->tokens;
	delete[] this->str;
}

void Parser::start() {
	LOG_DEBUG(LOG_EVADTS, "start");
	strLen = 0;
	symbolCR = false;
	error = false;
	procStart();
}

void Parser::procData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_EVADTS, "procData");
	LOG_DEBUG_STR(LOG_EVADTS, (const char*)data, len);
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
			if(addSymbol('\0') == false) {
				LOG_DEBUG(LOG_EVADTS, "Line termination failed");
				error = true;
				return;
			}
			if(parseTokens() == false) {
				LOG_DEBUG(LOG_EVADTS, "parseTokens failed");
				error = true;
				return;
			}
			if(procLine(tokens, tokenNum) == false) {
				LOG_DEBUG(LOG_EVADTS, "procLine failed");
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

void Parser::complete() {
	LOG_DEBUG(LOG_EVADTS, "complete");
	procComplete();
}

bool Parser::addSymbol(char symbol) {
	if(strLen >= EVADTS_LINE_MAX_SIZE) {
		LOG_ERROR(LOG_EVADTS, "Too long line");
		return false;
	}
	str[strLen] = symbol;
	strLen++;
	return true;
}

bool Parser::addToken(char *str) {
	if(tokenNum >= EVADTS_TOKEN_MAX_NUM) {
		LOG_ERROR(LOG_EVADTS, "Too much tokens");
		return false;
	}
	tokens[tokenNum] = str;
	tokenNum++;
	LOG_TRACE(LOG_EVADTS, "Token[" << tokenNum << "]" << str);
	return true;
}

bool Parser::parseTokens() {
	tokenNum = 0;
	addToken(str);
	uint16_t i = 0;
	while(str[i] != '\0') {
		if(str[i] == '*') {
			str[i] = '\0';
			i++;
			if(addToken(str + i) == false) { return false; }
		} else {
			i++;
		}
	}
	return true;
}

bool Parser::hasToken(uint16_t tokenIndex) {
	return (tokenNum >= (tokenIndex + 1));
}

bool Parser::hasTokenValue(uint16_t tokenIndex) {
	return (tokenNum >= (tokenIndex + 1) && tokens[tokenIndex][0] != '\0');
}

bool latinToWin1251(char *name) {
	char *from = name;
	char *to = name;
	uint8_t hex;
	char symbol;
	while(*from != '\0') {
		if(*from == '^') {
			from++;
			if(*from == '\0') {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			hex = hexToDigit(*from);
			if(hex > 15) {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			symbol = (hex << 4);

			from++;
			if(*from == '\0') {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			hex = hexToDigit(*from);
			if(hex > 15) {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			symbol |= hex;

			from++;
			if(*from != '^') {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
		} else {
			symbol = *from;
		}
		*to = symbol;
		to++;
		from++;
	}
	*to = '\0';
	return true;
}

}
