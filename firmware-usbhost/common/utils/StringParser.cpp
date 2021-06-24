#include <stdint.h>
#include <string.h>
#include <strings.h>

#include "include/StringParser.h"

#include "utils/include/Number.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

StringParser::StringParser(const char *string) :
	string(string),
	cur(0)
{
	size = strlen(string);
}

StringParser::StringParser(const char *string, uint16_t size) :
	string(string),
	size(size),
	cur(0)
{}

void StringParser::init(const char *string, uint16_t size) {
	this->string = string;
	this->size = size;
	this->cur = 0;
}

uint16_t StringParser::parsedLen() {
	return cur;
}

const char *StringParser::unparsed() {
	return (string + cur);
}

uint16_t StringParser::unparsedLen() {
	return (size - cur);
}

bool StringParser::hasUnparsed() {
	return (cur < size);
}

uint16_t StringParser::getIndex() {
	return cur;
}

void StringParser::reset() {
	cur = 0;
}

bool StringParser::compare(char value) {
	uint16_t valueLen = 1;
	if(unparsedLen() < valueLen) {
		return false;
	}
	if(value != string[cur]) {
		return false;
	}
	return true;
}

bool StringParser::compareAndSkip(char value) {
	uint16_t valueLen = 1;
	if(unparsedLen() < valueLen) {
		return false;
	}
	if(value != string[cur]) {
		return false;
	}
	cur += valueLen;
	return true;
}

bool StringParser::compare(const char *value) {
	uint16_t valueLen = strlen(value);
	if(unparsedLen() < valueLen) {
		return false;
	}
	if(strncasecmp(value, string + cur, valueLen) != 0) {
		return false;
	}
	return true;
}

bool StringParser::compareAndSkip(const char *value) {
	uint16_t valueLen = strlen(value);
	if(unparsedLen() < valueLen) {
		return false;
	}
	if(strncasecmp(value, string + cur, valueLen) != 0) {
		return false;
	}
	cur += valueLen;
	return true;
}

void StringParser::skipSpace() {
	while((string[cur] == ' ' || string[cur] == '\t') && cur < size) cur++;
}

void StringParser::skipWord() {
	while((string[cur] != ' ' && string[cur] != '\t') && cur < size) cur++;
}

void StringParser::skipEqual(const char *symbols) {
	for(; cur < size; cur++) {
		if(searchSymbol(symbols, string[cur]) == false) {
			return;
		}
	}
}

void StringParser::skipNotEqual(const char *symbols) {
	for(; cur < size; cur++) {
		if(searchSymbol(symbols, string[cur]) == true) {
			return;
		}
	}
}

void StringParser::skipNotEqual(const char *symbols, const char escapeSymbol) {
	bool escape = false;
	for(; cur < size; cur++) {
		char s = string[cur];
		if(escape == false) {
			if(s == escapeSymbol) {
				escape = true;
				continue;
			}
			if(searchSymbol(symbols, string[cur]) == true) {
				return;
			}
		} else {
			escape = false;
		}
	}
}

uint16_t StringParser::getValue(const char *delimiters, char *buf, uint16_t bufSize) {
	uint16_t valueLen = 0;
	while(1) {
		if(valueLen >= bufSize) {
			return valueLen;
		}
		if(cur >= size) {
			return valueLen;
		}
		for(const char *d = delimiters; *d != '\0'; d++) {
			if(string[cur] == *d) {
				return valueLen;
			}
		}
		buf[valueLen] = string[cur];
		valueLen++;
		cur++;
	}
	return valueLen;
}

bool StringParser::searchSymbol(const char *symbols, char symbol) {
	for(uint16_t i = 0; symbols[i] != '\0'; i++) {
		if(symbol == symbols[i]) {
			return true;
		}
	}
	return false;
}

