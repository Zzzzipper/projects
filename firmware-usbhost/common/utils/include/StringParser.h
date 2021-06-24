#ifndef STRING_PARSER_H
#define STRING_PARSER_H

#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <stdint.h>
#include <string.h>

class StringParser {
public:
	StringParser(const char *string);
	StringParser(const char *string, uint16_t size);
	void init(const char *string, uint16_t size);

	uint16_t parsedLen();
	const char *unparsed();
	uint16_t unparsedLen();
	bool hasUnparsed();

	uint16_t getIndex();

	void reset();

	bool compare(char value);
	bool compareAndSkip(char value);
	bool compare(const char *value);
	bool compareAndSkip(const char *value);
	void skipSpace();
	void skipWord();
	void skipEqual(const char *symbols);
	void skipNotEqual(const char *symbols);
	void skipNotEqual(const char *symbols, const char escapeSymbol);

	uint16_t getValue(const char *delimiters, char *buf, uint16_t bufSize);

	template<typename T>
	bool getNumber(T *number) {
		const char *from = string + cur;
		uint16_t l = Sambery::stringToNumber<T>(from, unparsedLen(), number);
		if(l == 0) {
			return false;
		}
		cur += l;
		return true;
	}
	template<typename T>
	bool getNumber(uint16_t n, T *number) {
		const char *from = string + cur;
		uint16_t l = Sambery::stringToNumber<T>(from, unparsedLen(), n, number);
		if(l < n) {
			return false;
		}
		cur += l;
		return true;
	}

private:
	const char *string;
	uint16_t size;
	uint16_t cur;

	bool searchSymbol(const char *symbols, char symbol);
};

#endif
