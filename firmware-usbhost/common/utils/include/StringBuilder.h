#ifndef __STRING_BUILDER_H
#define __STRING_BUILDER_H

#include "Allocator.h"

#include <stdint.h>

class StringBuilder {
public:
	StringBuilder();
	StringBuilder(uint32_t bufSize, uint8_t *buf);
	StringBuilder(const uint32_t minSize);
	StringBuilder(const uint32_t minSize, const uint32_t maxSize);
	StringBuilder(const char *str);
	StringBuilder(const char *str, const uint32_t strLen);
	StringBuilder(const StringBuilder &other);
	~StringBuilder();

	void set(const char *str);
	void set(const char *str, uint32_t index);
	StringBuilder &setLast(const char c);
	StringBuilder &add(const char c);
	StringBuilder &addStr(const char *str);
	StringBuilder &addStr(const char *str, uint32_t strLen);
	StringBuilder &addHex(const uint8_t number);
	StringBuilder &addBcd(const wchar_t wc);
	StringBuilder &toLowerCase();
	StringBuilder &toUpperCase();
	void cut(uint32_t index);

	const char *getString() const;
	uint32_t getLen() const;
	uint32_t getSize() const;
	bool isEmpty() const;
	uint8_t *getData();
	void setLen(uint32_t index);

	char operator[](const uint32_t i) const;
	StringBuilder &operator <<(const char *str);
	StringBuilder &operator <<(const char);
	StringBuilder &operator <<(const uint16_t);
	StringBuilder &operator <<(const uint32_t);
	StringBuilder &operator <<(const uint64_t);
	StringBuilder &operator <<(const int);
	StringBuilder &operator <<(const long);
	StringBuilder &operator <<(const float);
	StringBuilder &operator <<(const StringBuilder &other);
	StringBuilder &operator =(const StringBuilder &other);
	void operator =(const char *str);
	bool operator ==(const StringBuilder &other);
	bool operator ==(const char *other);
	bool operator !=(const StringBuilder &other);
	bool contains(const char *other);
	bool contains(const StringBuilder &other);

	void clear();

#ifdef AVR
	static StringBuilder load(const char *progmem);
#endif

private:
	Ephor::Allocator<char> *allocator;
	uint32_t size;
	uint32_t index;
	char *p;
};

typedef StringBuilder String;

#endif
