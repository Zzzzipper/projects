/*
 * Logger.h
 *
 * Created: 26.12.2015 15:03:08
 *  Author: Vladimir
 */

#ifndef COMMON_LOGGER_LOGGER_H_
#define COMMON_LOGGER_LOGGER_H_

/*
 * Как использовать:
 * Каждый модуль задает свою категорию с помощью макроса вида LOG_<название-категории>, где
 * название категории - это сокращенное название модуля, желательно 3 или 4 символа.
 * Значение макроса от 0 до 5 определяет детализацию лога.
 * Сам макрос обязательно должен быть задан в файле config.h, иначе будет ошибка.
 * В библиотеке COMMON ни в коем случае нельзя задавать конкретные значения категории лога модуля,
 * так как это повлечет за собой правки в репозитории.
 * Если значение макроса ниже уровня лога макроса, то компилятор при оптимизации не включит код в сборку.
 * Если макрос LOGGING не задан, то макросы будут заменены на пустышки и не будут анализироваться вообще.
 *
 * Пример кода:
 * модуль DEX
 * макрос категории лога LOG_DEX
 * значение макроса LOG_DEX=3, задано через опции компилятора
 * запись в коде LOG_WARN(LOD_DEX, "Внимание!");
 *
 * Преимущества подхода:
 * 1. Набор макросов один на все модули;
 * 2. Каждый модуль имеет свой набор уровней детализации;
 * 3. Детализация может быть включена только для конкретных модулей;
 * 4. Детализация лога настраивается для каждого проекта индивидуально;
 * 5. Настройки детализации не сохраняются в библиотеку COMMON;
 * 6. Все логи можно отключить одним макросом.
 */

#define LOG_LEVEL_OFF	0 // Лог данной категории отключен
#define LOG_LEVEL_ERROR	1 // Уровень серьезных ошибок и ключевых событий
#define LOG_LEVEL_WARN	2 // Нештатные ситуации и важные события
#define LOG_LEVEL_INFO	3 // Шаги автоматов и основные действия
#define LOG_LEVEL_DEBUG	4 // Отладочная информация
#define LOG_LEVEL_TRACE	5 // Пошаговый или побайтовый вывод данных

#include "config.h" // данный файл должен лежать в корне проекта (не COMMON) и настраивать логи всех модулей проекта: как общих(COMMON), так специальных
#include "utils/include/Utils.h"
#include <timer/include/RealTime.h>
#include <stdint.h>

#ifdef _MSC_VER
#ifdef LOGGING
// нежелательно использовать методы LOG, LOG_HEX и LOG_STR напрямую

#ifdef LOGGING_TIMESTAMP
#define LOG_TIMESTAMP << Logger::datetime
#else
#define LOG_TIMESTAMP
#endif

#ifdef LOGGING_FILELINE
#define LOG_FILENAME << basename(__FILE__) << "#" << __LINE__ << " "
#else
#define LOG_FILENAME
#endif

#ifdef LOGGING_LEVEL
#define LOG_PFX(__prefix, ...) *(Logger::get()) << __prefix LOG_TIMESTAMP LOG_FILENAME << __VA_ARGS__
#else
#define LOG_PFX(__prefix, ...) *(Logger::get()) LOG_TIMESTAMP LOG_FILENAME << __VA_ARGS__
#endif

#define LOG(...) *(Logger::get()) LOG_TIMESTAMP LOG_FILENAME << __VA_ARGS__ << Logger::endl
#define LOG_HEX(...) Logger::get()->hex(__VA_ARGS__); *(Logger::get()) << Logger::endl
#define LOG_STR(...) Logger::get()->str(__VA_ARGS__); *(Logger::get()) << Logger::endl
#define LOG_HEX_WIRESHARK(...) Logger::get()->toWiresharkHex(__VA_ARGS__); *(Logger::get()) << Logger::endl

#define LOG_ERROR(LOG_CATEGORY, ...)    	if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_PFX("E ", __VA_ARGS__) << Logger::endl; }
#define LOG_ERROR_WORD(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_PFX("E ", __VA_ARGS__); }
#define LOG_ERROR_HEX(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_HEX(__VA_ARGS__); }
#define LOG_ERROR_STR(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_STR(__VA_ARGS__); }
#define LOG_WARN(LOG_CATEGORY, ...)    	if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_PFX("W ", __VA_ARGS__) << Logger::endl; }
#define LOG_WARN_WORD(LOG_CATEGORY, ...)   if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_PFX("W ", __VA_ARGS__); }
#define LOG_WARN_HEX(LOG_CATEGORY, ...)   if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_HEX(__VA_ARGS__); }
#define LOG_WARN_STR(LOG_CATEGORY, ...)   if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_STR(__VA_ARGS__); }
#define LOG_INFO(LOG_CATEGORY, ...)    	if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_PFX("I ", __VA_ARGS__) << Logger::endl; }
#define LOG_INFO_WORD(LOG_CATEGORY, ...)   if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_PFX("I ", __VA_ARGS__); }
#define LOG_INFO_HEX(LOG_CATEGORY, ...)   if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_HEX(__VA_ARGS__); }
#define LOG_INFO_STR(LOG_CATEGORY, ...)   if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_STR(__VA_ARGS__); }
#define LOG_DEBUG(LOG_CATEGORY, ...)    	if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_PFX("D ", __VA_ARGS__) << Logger::endl; }
#define LOG_DEBUG_WORD(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_PFX("D ", __VA_ARGS__); }
#define LOG_DEBUG_HEX(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_HEX(__VA_ARGS__); }
#define LOG_DEBUG_STR(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_STR(__VA_ARGS__); }
#define LOG_TRACE(LOG_CATEGORY, ...)    	if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_PFX("T ", __VA_ARGS__) << Logger::endl; }
#define LOG_TRACE_WORD(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_PFX("T ", __VA_ARGS__); }
#define LOG_TRACE_HEX(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_HEX(__VA_ARGS__); }
#define LOG_TRACE_STR(LOG_CATEGORY, ...)  if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_STR(__VA_ARGS__); }

#else
#define LOG(...)
#define LOG_PFX(__prefix, ...)
#define LOG_HEX(...)
#define LOG_STR(...)
#define LOG_HEX_WIRESHAR(...)

#define LOG_ERROR(LOG_CATEGORY, ...)
#define LOG_ERROR_WORD(LOG_CATEGORY, ...)
#define LOG_ERROR_HEX(LOG_CATEGORY, ...)
#define LOG_ERROR_STR(LOG_CATEGORY, ...)
#define LOG_WARN(LOG_CATEGORY, ...)
#define LOG_WARN_WORD(LOG_CATEGORY, ...)
#define LOG_WARN_HEX(LOG_CATEGORY, ...)
#define LOG_WARN_STR(LOG_CATEGORY, ...)
#define LOG_INFO(LOG_CATEGORY, ...)
#define LOG_INFO_WORD(LOG_CATEGORY, ...)
#define LOG_INFO_HEX(LOG_CATEGORY, ...)
#define LOG_INFO_STR(LOG_CATEGORY, ...)
#define LOG_DEBUG(LOG_CATEGORY, ...)
#define LOG_DEBUG_WORD(LOG_CATEGORY, ...)
#define LOG_DEBUG_HEX(LOG_CATEGORY, ...)
#define LOG_DEBUG_STR(LOG_CATEGORY, ...)
#define LOG_TRACE(LOG_CATEGORY, ...)
#define LOG_TRACE_WORD(LOG_CATEGORY, ...)
#define LOG_TRACE_HEX(LOG_CATEGORY, ...)
#define LOG_TRACE_STR(LOG_CATEGORY, ...)
#endif
#else
#ifdef __linux__
#include <string.h>
#endif
#ifdef LOGGING
// нежелательно использовать методы LOG, LOG_HEX и LOG_STR напрямую

#ifdef LOGGING_TIMESTAMP
#define LOG_TIMESTAMP << Logger::datetime
#else
#define LOG_TIMESTAMP
#endif

#ifdef LOGGING_FILELINE
#define LOG_FILENAME << basename(__FILE__) << "#" << __LINE__ << " "
#else
#define LOG_FILENAME
#endif

#ifndef LOG_PFX_DISABLE
#ifdef LOGGING_LEVEL
#define LOG_PFX(__prefix, __args...) *(Logger::get()) << __prefix LOG_TIMESTAMP LOG_FILENAME << __args
#else
#define LOG_PFX(__prefix, __args...) *(Logger::get()) LOG_TIMESTAMP LOG_FILENAME << __args
#endif
#else
#define LOG_PFX(__prefix, __args...) *(Logger::get()) << __args
#endif

#define LOG(__args...) *(Logger::get()) LOG_TIMESTAMP LOG_FILENAME << __args << Logger::endl
#define LOG_HEX(__args...) Logger::get()->hex(__args); *(Logger::get()) << Logger::endl
#define LOG_STR(__args...) Logger::get()->str(__args); *(Logger::get()) << Logger::endl
#define LOG_HEX_WIRESHARK(__args...) Logger::get()->toWiresharkHex(__args); *(Logger::get()) << Logger::endl

#define LOG_ERROR(LOG_CATEGORY, __args...)    	if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_PFX("E ", __args) << Logger::endl; }
#define LOG_ERROR_WORD(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_PFX("E ", __args); }
#define LOG_ERROR_HEX(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_HEX(__args); }
#define LOG_ERROR_STR(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { LOG_STR(__args); }
#define LOG_WARN(LOG_CATEGORY, __args...)    	if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_PFX("W ", __args) << Logger::endl; }
#define LOG_WARN_WORD(LOG_CATEGORY, __args...)   if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_PFX("W ", __args); }
#define LOG_WARN_HEX(LOG_CATEGORY, __args...)   if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_HEX(__args); }
#define LOG_WARN_STR(LOG_CATEGORY, __args...)   if(LOG_CATEGORY >= LOG_LEVEL_WARN)  { LOG_STR(__args); }
#define LOG_INFO(LOG_CATEGORY, __args...)    	if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_PFX("I ", __args) << Logger::endl; }
#define LOG_INFO_WORD(LOG_CATEGORY, __args...)   if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_PFX("I ", __args); }
#define LOG_INFO_HEX(LOG_CATEGORY, __args...)   if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_HEX(__args); }
#define LOG_INFO_STR(LOG_CATEGORY, __args...)   if(LOG_CATEGORY >= LOG_LEVEL_INFO)  { LOG_STR(__args); }
#define LOG_DEBUG(LOG_CATEGORY, __args...)    	if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_PFX("D ", __args) << Logger::endl; }
#define LOG_DEBUG_WORD(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_PFX("D ", __args); }
#define LOG_DEBUG_HEX(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_HEX(__args); }
#define LOG_DEBUG_STR(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_DEBUG) { LOG_STR(__args); }
#define LOG_TRACE(LOG_CATEGORY, __args...)    	if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_PFX("T ", __args) << Logger::endl; }
#define LOG_TRACE_WORD(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_PFX("T ", __args); }
#define LOG_TRACE_HEX(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_HEX(__args); }
#define LOG_TRACE_STR(LOG_CATEGORY, __args...)  if(LOG_CATEGORY >= LOG_LEVEL_TRACE) { LOG_STR(__args); }

#else
#define LOG(__args...)
#define LOG_PFX(__prefix, __args...)
#define LOG_HEX(__args...)
#define LOG_STR(__args...)
#define LOG_HEX_WIRESHAR(__args...)

#define LOG_ERROR(LOG_CATEGORY, __args...)
#define LOG_ERROR_WORD(LOG_CATEGORY, __args...)
#define LOG_ERROR_HEX(LOG_CATEGORY, __args...)
#define LOG_ERROR_STR(LOG_CATEGORY, __args...)
#define LOG_WARN(LOG_CATEGORY, __args...)
#define LOG_WARN_WORD(LOG_CATEGORY, __args...)
#define LOG_WARN_HEX(LOG_CATEGORY, __args...)
#define LOG_WARN_STR(LOG_CATEGORY, __args...)
#define LOG_INFO(LOG_CATEGORY, __args...)
#define LOG_INFO_WORD(LOG_CATEGORY, __args...)
#define LOG_INFO_HEX(LOG_CATEGORY, __args...)
#define LOG_INFO_STR(LOG_CATEGORY, __args...)
#define LOG_DEBUG(LOG_CATEGORY, __args...)
#define LOG_DEBUG_WORD(LOG_CATEGORY, __args...)
#define LOG_DEBUG_HEX(LOG_CATEGORY, __args...)
#define LOG_DEBUG_STR(LOG_CATEGORY, __args...)
#define LOG_TRACE(LOG_CATEGORY, __args...)
#define LOG_TRACE_WORD(LOG_CATEGORY, __args...)
#define LOG_TRACE_HEX(LOG_CATEGORY, __args...)
#define LOG_TRACE_STR(LOG_CATEGORY, __args...)
#endif
#endif

#if defined(LOGGING) && defined(LOGGING_MEMORY)
#define LOG_MEMORY_USAGE(__title) LOG_PFX("M ", __title) << " " << Logger::memoryUsage << Logger::endl
#else
#define LOG_MEMORY_USAGE(__title)
#endif

class LogTarget {
public:
	virtual ~LogTarget() {}
	virtual void send(const uint8_t *data, const uint16_t len) = 0;
};

class Logger {
public:
	static Logger *get();
	void registerTarget(LogTarget *logTarget);
	void registerRealTime(RealTimeInterface *realtime);
	LogTarget *getTarget() { return logTarget; }
	Logger &operator<<(const char symbol);
	Logger &operator<<(const char *str);
	Logger &operator<<(const uint8_t num);
	Logger &operator<<(const uint16_t num);
	Logger &operator<<(const uint32_t num);
	Logger &operator<<(const uint64_t num);
	Logger &operator<<(const int64_t num);
	Logger &operator<<(const int num);
#ifndef __linux__
	Logger &operator<<(const long value);
#endif
	Logger &operator<<(const float value);
	Logger &operator<<(Logger &(*func)(Logger &));
	Logger &operator<<(Logger &logger);

	static Logger &datetime(Logger &);
	static Logger &endl(Logger &);
	Logger &hex(const uint8_t symbol);
	Logger &hex(const uint8_t *data, uint16_t len);
	Logger &hex16(const uint16_t value);
	Logger &hex32(const uint32_t value);
	void toWiresharkHex(const uint8_t *data, uint16_t len);
	void str(const char symbol);
	void str(const char *data, uint16_t len);
	void str(const uint8_t *data, uint16_t len);
	void setRamOffset(uint32_t offset);
	void setRamSize(uint32_t size);
	static Logger &memoryUsage(Logger &);

	struct stMemoryUsage
	{
		uint32_t allocated; // Сколько выделено памяти системой
		uint32_t used; // Сколько из нее используется
		uint32_t non_inuse; // Сколько не используется. Индикатор фрагментации памяти
		uint32_t free; // Сколько свободно памяти
		uint32_t currentStackSize; // Размер стека в момент вызова
	};

	struct stMemoryUsage getMemoryUsage();

private:
	LogTarget *logTarget;
	RealTimeInterface *realtime;
	uint32_t ramOffset;
	uint32_t ramSize;

	Logger();
	~Logger();
	Logger(const Logger &c);
	Logger& operator=(const Logger &c);
	static Logger *instance;
};

#endif
