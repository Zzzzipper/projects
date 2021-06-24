#include "include/StringBuilder.h"
#include "platform/include/platform.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN_BUF_SIZE 2
#define DEFAULT_MIN_BUF_SIZE 17
#define DEFAULT_MAX_BUF_SIZE 1025
#define INC_BUF_SIZE 16

const char ru[] = "йцукенгшщзхъфывапролджэячсмитьбю";
const char RU[] = "ЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮ";

StringBuilder::StringBuilder() {
	allocator = new Ephor::DynamicAllocator<char>(DEFAULT_MIN_BUF_SIZE, DEFAULT_MAX_BUF_SIZE);
	p = allocator->init(&size);
	index = 0;
	p[index] = '\0';
}

StringBuilder::StringBuilder(uint32_t bufSize, uint8_t *buf) {
	allocator = new Ephor::StaticAllocator<char>(buf, bufSize);
	p = allocator->init(&size);
	index = 0;
	p[index] = '\0';
}

StringBuilder::StringBuilder(const uint32_t minSize) {
	allocator = new Ephor::DynamicAllocator<char>(minSize + 1, DEFAULT_MAX_BUF_SIZE);
	p = allocator->init(&size);
	index = 0;
	p[index] = '\0';
}

StringBuilder::StringBuilder(const uint32_t minSize, const uint32_t maxSize) {
	allocator = new Ephor::DynamicAllocator<char>(minSize + 1, maxSize + 1);
	p = allocator->init(&size);
	index = 0;
	p[index] = '\0';
}

StringBuilder::StringBuilder(const char *str) {
	uint32_t strLen = strlen(str);
	allocator = new Ephor::DynamicAllocator<char>(strLen + 1, DEFAULT_MAX_BUF_SIZE);
	p = allocator->init(&size);
	index = 0;
	p[index] = '\0';
	set(str, strLen);
}

StringBuilder::StringBuilder(const char *str, const uint32_t strLen) {
	allocator = new Ephor::DynamicAllocator<char>(strLen + 1, DEFAULT_MAX_BUF_SIZE);
	p = allocator->init(&size);
	index = 0;
	p[index] = '\0';
	set(str, strLen);
}

StringBuilder::StringBuilder(const StringBuilder &other) {
	allocator = new Ephor::DynamicAllocator<char>(DEFAULT_MIN_BUF_SIZE, DEFAULT_MAX_BUF_SIZE);
	p = allocator->init(&size);
	index = 0;
	p[index] = '\0';
	set(other.p, other.index);
}

StringBuilder::~StringBuilder() {
	delete allocator;
}

void StringBuilder::set(const char *str) {
	uint32_t len = strlen(str);
	set(str, len);
}

void StringBuilder::set(const char *str, uint32_t strLen) {
	uint32_t copyLen = strLen;
	if(copyLen >= size) {
		p = allocator->resize(&size, copyLen + 1);
		if(copyLen >= size) {
			copyLen = size - 1;
		}
	}
	for(index = 0; index < copyLen; index++) {
		p[index] = str[index];
	}
	p[index] = '\0';
	return;
}

StringBuilder &StringBuilder::setLast(const char c) {
	if(c == '\0') return *this;
	if(index <= 0) return *this;
	p[index - 1] = c;
	return *this;
}

StringBuilder &StringBuilder::add(const char c) {
	if(c == '\0') return *this;
	uint32_t copyLen = index + 1;
	if(copyLen >= size) {
		p = allocator->resize(&size, size + INC_BUF_SIZE);
		if(copyLen >= size) {
			return *this;
		}
	}
	p[index] = c;
	index++;
	p[index] = '\0';
	return *this;
}

StringBuilder &StringBuilder::addStr(const char *str) {
	while(*str) {
		add(*str);
		str++;
	}

	return *this;
}

StringBuilder &StringBuilder::addStr(const char *str, uint32_t strLen) {
	for(uint32_t i = 0; i < strLen; i++) {
		add(str[i]);
	}
	return *this;
}

StringBuilder &StringBuilder::addHex(const uint8_t symbol) {
	static char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	return *this << hex[symbol/16] << hex[symbol%16];
}

StringBuilder &StringBuilder::addBcd(const wchar_t wc) {
	char c[8];
	sprintf(c, "%.4X", wc);
	addStr(c);
	return *this;
}

StringBuilder &StringBuilder::toLowerCase() {
	for(uint32_t i = 0; i < index; i++) {
		char c = p[i];
		if(c >= 'A' && c <= 'Z') {
			p[i] = c - 'A' + 'a';
			continue;
		}

		for(uint8_t r = 0; r < sizeof(ru); r++) {
			if(c == RU[r]) {
				p[i] = ru[r];
				break;
			}
		}
	}
	return *this;
}

StringBuilder &StringBuilder::toUpperCase() {
	for(uint32_t i = 0; i < index; i++) {
		char c = p[i];
		if(c >= 'a' && c <= 'z') {
			p[i] = c - 'a' + 'A';
			continue;
		}

		for (uint8_t r = 0; r < sizeof(ru); r++) {
			if (c == ru[r]) {
				p[i] = RU[r];
				break;
			}
		}
	}
	return *this;
}

void StringBuilder::cut(uint32_t len) {
	if(len >= index) {
		return;
	}
	index = len;
	p[index] = '\0';
}

char StringBuilder::operator [](const uint32_t i) const {
	if(i >= size) {
		return p[size - 1];
	}
	return p[i];
}

StringBuilder & StringBuilder::operator <<(const char *str) {
	addStr(str);
	return *this;
}

StringBuilder & StringBuilder::operator <<(const char value) {
	add(value);
	return *this;
}

StringBuilder & StringBuilder::operator <<(const int value) {
	char c[16];
	uint32_t len = Sambery::numberToString<int>(value, c, sizeof(c));
	addStr(c, len);
	return *this;
}

StringBuilder & StringBuilder::operator <<(const uint16_t value) {
	char c[16];
	uint32_t len = Sambery::numberToString<uint16_t>(value, c, sizeof(c));
	addStr(c, len);
	return *this;
}

StringBuilder & StringBuilder::operator <<(const uint32_t value) {
	char c[16];
	uint32_t len = Sambery::numberToString<uint32_t>(value, c, sizeof(c));
	addStr(c, len);
	return *this;
}

StringBuilder & StringBuilder::operator <<(const uint64_t value) {
	char c[32];
	uint32_t len = Sambery::numberToString<uint64_t>(value, c, sizeof(c));
	addStr(c, len);
	return *this;
}

StringBuilder & StringBuilder::operator <<(const long value) {
	char c[16];
	uint32_t len = Sambery::numberToString<long>(value, c, sizeof(c));
	addStr(c, len);
	return *this;
}

StringBuilder & StringBuilder::operator <<(const float value) {

	operator <<((int) value);

	// FIXME: Нужно доделать. По неизвестным причинам, код ниже работает только из прерывания. В остальных случаях в строке будет содержаться 0.00 или мусор!
	//	char c[32];
	//	snprintf(c, 32, "%.2f", (double)value);
	//	addStr(c);

	return *this;
}

StringBuilder & StringBuilder::operator <<(const StringBuilder &other) {
	this->operator <<(other.getString());
	return *this;
}

StringBuilder & StringBuilder::operator =(const StringBuilder &other) {
	this->operator =(other.getString());
	return *this;
}

void StringBuilder::operator =(const char *str) {
	set(str);
}

bool StringBuilder::isEmpty() const {
	return index == 0;
}

const char *StringBuilder::getString() const {
	return p;
}

uint32_t StringBuilder::getLen() const {
	return index;
}

uint32_t StringBuilder::getSize() const {
	return (size - 1);
}

/*
 * Позволяет напрямую писать в буфер строки.
 * Метод очень опасный, будьте осторожны!
 */
uint8_t *StringBuilder::getData() {
	return (uint8_t*)p;
}

/*
 * Устанавливает длину строки. Не изменяет размер!
 * Используется в паре с getString().
 */
void StringBuilder::setLen(uint32_t len) {
	if(len >= size) {
		index = size - 1;
		p[index] = '\0';
	} else {
		index = len;
		p[index] = '\0';
	}
}

void StringBuilder::clear() {
	p = allocator->resize(&size, 0);
	index = 0;
	p[index] = '\0';
}

bool StringBuilder::operator ==(const StringBuilder &other) {
	return (strcmp(getString(), other.getString()) == 0);
}

bool StringBuilder::operator ==(const char *other) {
	return (strcmp(getString(), other) == 0);
}

bool StringBuilder::operator !=(const StringBuilder &other) {
	return (strcmp(getString(), other.getString()) != 0);
}

bool StringBuilder::contains(const char *other) {
	return (strstr(getString(), other));
}

bool StringBuilder::contains(const StringBuilder &other) {
	return (strstr(getString(), other.getString()));
}

// =====================================================================
#ifdef AVR // этому методу тут не место, это должен быть отдельный класс
StringBuilder StringBuilder::load(const char *progmem)
{
	StringBuilder str;

	char c;
	while ((c = pgm_read_byte(progmem++)))	str.add(c);

	return str;
}
#endif
