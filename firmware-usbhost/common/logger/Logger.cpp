#include "include/Logger.h"

#include "platform/include/platform.h"
#include "utils/include/Number.h"
#include "utils/include/Hex.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DIGIT_MAX_SIZE 22

Logger* Logger::instance = NULL;

Logger *Logger::get() {
	if(instance == NULL) {
		instance = new Logger();
	}
	return instance;
}

Logger::Logger() :
	logTarget(NULL),
	realtime(NULL),
	ramOffset(0)
{
}

Logger::~Logger() {
	if(instance != NULL) {
		delete instance;
	}
}

void Logger::registerTarget(LogTarget *logTarget) {
	this->logTarget = logTarget;
}

void Logger::registerRealTime(RealTimeInterface *realtime) {
	this->realtime = realtime;
}

Logger &Logger::operator<<(const char symbol) {
	if(this->logTarget == NULL) {
		return *this;
	}
	this->logTarget->send(reinterpret_cast<const uint8_t*>(&symbol), 1);
	return *this;
}

Logger &Logger::operator<<(const char *str) {
	if(this->logTarget == NULL) {
		return *this;
	}
	int len = strlen(str);
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}

Logger &Logger::operator<<(const uint8_t num) {
	if(this->logTarget == NULL) {
		return *this;
	}
	char str[DIGIT_MAX_SIZE];
	uint16_t len = Sambery::numberToString<uint8_t>(num, str, sizeof(str));
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}

Logger &Logger::operator<<(const uint16_t num) {
	if(this->logTarget == NULL) {
		return *this;
	}
	char str[DIGIT_MAX_SIZE];
	uint16_t len = Sambery::numberToString<uint16_t>(num, str, sizeof(str));
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}

Logger &Logger::operator<<(const uint32_t num) {
	if(this->logTarget == NULL) {
		return *this;
	}
	char str[DIGIT_MAX_SIZE];
	uint16_t len = Sambery::numberToString<uint32_t>(num, str, sizeof(str));
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}

Logger &Logger::operator<<(const uint64_t num) {
	if(this->logTarget == NULL) {
		return *this;
	}
	char str[DIGIT_MAX_SIZE];
	uint16_t len = Sambery::numberToString<uint64_t>(num, str, sizeof(str));
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}

Logger &Logger::operator<<(const int64_t num) {
	if(this->logTarget == NULL) {
		return *this;
	}
	char str[DIGIT_MAX_SIZE];
	uint16_t len = Sambery::numberToString<int64_t>(num, str, sizeof(str));
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}

Logger &Logger::operator<<(const int num) {
	if(this->logTarget == NULL) {
		return *this;
	}
	char str[DIGIT_MAX_SIZE];
	uint16_t len = Sambery::numberToString<int>(num, str, sizeof(str));
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}
#ifndef __linux__
Logger &Logger::operator<<(const long num) {
	if(this->logTarget == NULL) {
		return *this;
	}
	char str[DIGIT_MAX_SIZE];
	uint16_t len = Sambery::numberToString<long>(num, str, sizeof(str));
	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}
#endif

Logger &Logger::operator<<(const float num) {
	if(this->logTarget == NULL) {
		return *this;
	}

	operator <<(int(num));
// FIXME: Ќужно доделать. ѕо неизвестным причинам, код ниже работает только из прерывани€. ¬ остальных случа€х в строке будет содержатьс€ 0.00 или мусор!
//	char str[DIGIT_MAX_SIZE];
//	snprintf(str, DIGIT_MAX_SIZE, "%.4f", num);
//	uint16_t len = strlen(str);
//	this->logTarget->send(reinterpret_cast<const uint8_t*>(str), len);
	return *this;
}

Logger &Logger::operator<<(Logger &(*func)(Logger &)) {
	return func(*this);
}

Logger &Logger::operator<<(Logger &logger) {
	(void)logger;
	return *this;
}

Logger &Logger::datetime(Logger &me) {
	if(me.realtime == NULL) {
		return me;
	}
	DateTime d;
	me.realtime->getDateTime(&d);
	char b[16];
	sprintf(b, "%.2d:%.2d:%.2d ", d.hour, d.minute, d.second);
	me << b;
	return me;
}

Logger &Logger::endl(Logger &me) {
	me << "\r\n";
	return me;
}

Logger &Logger::hex(const uint8_t symbol) {
	*this << "x" << digitToHex(symbol/16) << digitToHex(symbol%16) << ";";
	return *this;
}

Logger &Logger::hex(const uint8_t *data, uint16_t len) {
	for(uint16_t i = 0; i < len; i++) {
		this->hex(data[i]);
	}
	return *this;
}

Logger &Logger::hex16(const uint16_t value)
{
	char b[16];
	sprintf(b, "0x%.4X", value);
	*this << b;
	return *this;
}

Logger &Logger::hex32(const uint32_t value)
{
	char b[16];
	sprintf(b, "0x%.8X", value);
	*this << b;
	return *this;
}

void Logger::toWiresharkHex(const uint8_t *data, uint16_t len) {
	char b[16];
	for (uint16_t i = 0; i < len; i += 16) {
		sprintf(b, "%.6X ", i);
		*this << b;
		for (uint16_t i2 = i; i2 < i + 16 && i2 < len; i2++) {
			*this << digitToHex(data[i2]/16) << digitToHex(data[i2]%16) << " ";
		}
		*this << "\r\n";
	}
}

void Logger::str(const char symbol) {
	*this << symbol;
}

void Logger::str(const char *data, uint16_t len) {
	if(this->logTarget == NULL) {
		return;
	}
	this->logTarget->send((uint8_t*)data, len);
}

void Logger::str(const uint8_t *data, uint16_t len) {
	if(this->logTarget == NULL) {
		return;
	}
	this->logTarget->send(data, len);
}

void Logger::setRamOffset(uint32_t offset) {
	ramOffset = offset;
}

void Logger::setRamSize(uint32_t size)
{
	ramSize = size;
}

#include <malloc.h>
// Ётому коду не место в этом файле. Ёто должен быть отдельный класс, дл€ определени€ текущего состо€ни€ пам€ти.
// ѕричем класс должен быть спроектирован как кроссплатформенный.
struct Logger::stMemoryUsage Logger::getMemoryUsage() {
#ifdef ARM
	struct mallinfo info = mallinfo();
	struct stMemoryUsage result;
	result.currentStackSize = ramOffset + ramSize - __get_MSP();
	result.allocated = info.arena;
	result.used = info.uordblks;
	result.non_inuse = info.fordblks;
	result.free = __get_MSP() - ramOffset - result.allocated;
	return result;
#else
	struct stMemoryUsage result;
	result.currentStackSize = 0;
	result.allocated = 0;
	result.used = 0;
	result.non_inuse = 0;
	result.free = 0;
	return result;
#endif
}

Logger &Logger::memoryUsage(Logger &me) {
	struct stMemoryUsage result = me.getMemoryUsage();
	me << "RAM, Allocated: " << result.allocated << ", used: " << result.used << ", non-inuse: " << result.non_inuse << ", stack size: " << result.currentStackSize << ", free: " << result.free;
	return me;
}
