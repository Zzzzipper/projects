#ifndef COMMON_ECP_PROTOCOL_H
#define COMMON_ECP_PROTOCOL_H

#include "utils/include/NetworkProtocol.h"
#include "utils/include/Buffer.h"

namespace Ecp {

/*
---------------------
Особенности протокола
---------------------
Версия 1.0.0

Структура блока команды и ответа:
	<STX><length><command><data><CRC>, где
	<length> - длина пакет. В длину не входит STX, <length> и <crc> байты;
	<command> - код команды;
	<CRC> - байт контрольной суммы

В варианте ETHERNET не используются байты STX и <crc>, т.к. всю «заботу»
об целостности принимаемых и передаваемых пакетов берет на себя стек TCP/IP.
Схема обмена так же не использует подтверждения ACK, NAK, ENQ.
Хост должен установить соединение с устройством на 3333 порту, отправить
в порт сообщение и ожидать ответа от устройства достаточно времени,
что бы устройство успело проделать работу определяемую командой.
После получения ответа хост должен закрыть соединение.
*/

#define ECP_PACKET_MAX_SIZE		200		// уточнить
#define ECP_CONNECT_TIMEOUT		100
#define ECP_CONNECT_TRIES		500
#define ECP_DISCONNECT_TIMEOUT  100
#define ECP_KEEP_ALIVE_PERIOD	500
#define ECP_KEEP_ALIVE_TIMEOUT	ECP_KEEP_ALIVE_PERIOD * 20 // вернуть множитель 2, когда ECP будет реализован в реальном времени
#define ECP_PACKET_TIMEOUT		1000	// время ожидания данных пакета
#define ECP_CONFIRM_TIMEOUT		10000	// время ожидания ответа (увеличенно из-за очистки флэш-памяти, уменьшить)
#define ECP_BUSY_TIMEOUT        10
#define ECP_DELIVER_TIMEOUT		1

enum Control {
	Control_STX  = 0x02,
	Control_EOT  = 0x04,
	Control_ENQ  = 0x05,
	Control_ACK  = 0x06,
	Control_NAK  = 0x15,
};

enum Command {
	Command_Setup			= 0x01, // Согласование версий протоколов
	Command_UploadStart		= 0x02, // Запрос на начало передачи данных
	Command_UploadData		= 0x03, // Передача очередного блока
	Command_UploadEnd		= 0x04, // Завершение передачи
	Command_UploadCancel	= 0x05, // Отмена передачи данных
	Command_DownloadStart	= 0x06, // Запрос на начало загрузки данных
	Command_DownloadData	= 0x07, // Загрузка очередного блока
	Command_DownloadCancel	= 0x08, // Отмена загрузки данных
	Command_TableInfo		= 0x09, // Загрузка информации о таблице
	Command_TableEntry		= 0x0A, // Загрузка записи таблицы
	Command_DateTime		= 0x0B, // Запрос даты и времени модема
	Command_ConfigReset		= 0x0C, // Сброс конфигурации модема
};

enum Error {
	Error_OK				= 0x00,  // Нет ошибок
	Error_Busy              = 0x01,
	Error_Disconnect        = 0x02,
	Error_TooManyTries		= 0x03,
	Error_Timeout			= 0x04,
	Error_WrongPacketSize   = 0x05,
	Error_WrongDestination	= 0x06,
	Error_WrongSource		= 0x07,
	Error_ServerError       = 0x08,
	Error_TableNotFound     = 0x09,
	Error_EntryNotFound     = 0x0A,
	Error_EndOfFile			= 0x0B,
};

class TableProcessor {
public:
	virtual bool isTableExist(uint16_t tableId) = 0;
	virtual uint32_t getTableSize(uint16_t tableId) = 0;
	virtual uint16_t getTableEntry(uint16_t tableId, uint32_t entryIndex, uint8_t *buf, uint16_t bufSize) = 0;
	virtual uint16_t getDateTime(uint8_t *buf, uint16_t bufSize) = 0;
};

class ClientPacketLayerInterface {
public:
	class Observer {
	public:
		virtual void procConnect() = 0;
		virtual void procRecvData(const uint8_t *data, uint16_t dataLen) = 0;
		virtual void procRecvError(Error error) = 0;
		virtual void procDisconnect() = 0;
	};

	virtual void setObserver(Observer *observer) = 0;
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual bool sendData(const Buffer *data) = 0;
};

class ServerPacketLayerInterface {
public:
	class Observer {
	public:
		virtual void procConnect() = 0;
		virtual void procRecvData(const uint8_t *data, uint16_t dataLen) = 0;
		virtual void procError(uint8_t code) = 0;
		virtual void procDisconnect() = 0;
	};

	virtual void setObserver(Observer *observer) = 0;
	virtual void reset() = 0;
	virtual void shutdown() = 0;
	virtual void disconnect() = 0;
	virtual bool sendData(const Buffer *data) = 0;
};

class Crc {
public:
	void start(uint8_t byte) { crc = byte; }
	void add(uint8_t byte) { crc = crc ^ byte; }
	uint8_t getCrc() { return crc; }
	static uint8_t calc(const Buffer *data) {
		uint8_t dataLen = data->getLen();
		Crc crc;
		crc.start(dataLen);
		for(uint8_t i = 0; i < dataLen; i++) {
			crc.add((*data)[i]);
		}
		return crc.getCrc();
	}

private:
	uint8_t crc;
};

#pragma pack(push,1)
/*
 * Стандартная команда без дополнительных параметров.
 */
struct Request {
	uint8_t command;
};

struct Response {
	uint8_t command;
	uint8_t errorCode;  // Код ошибки
};

enum Destination {
	Destination_FirmwareGsm		= 0x01,
	Destination_FirmwareModem	= 0x02,
	Destination_Config			= 0x03,
	Destination_FirmwareScreen	= 0x04,
};

struct UploadStartRequest {
	uint8_t command;
	uint8_t destination;
	uint32_t dataSize;
};

struct UploadDataRequest {
	uint8_t command;
	uint8_t data[0];
};

enum Source {
	Source_Audit  = 0x01,
	Source_Config = 0x02,
};

struct DownloadStartRequest {
	uint8_t command;
	uint8_t source;
};

struct DownloadStartResponse {
	uint8_t command;
	uint8_t errorCode;
	uint32_t dataSize;
};

struct DownloadDataResponse {
	uint8_t command;
	uint8_t errorCode;
	uint8_t data[0];
};

enum Table {
	Table_Event = 0x0001,
};

struct TableInfoRequest {
	uint8_t command;
	uint16_t tableId;
};

struct TableInfoResponse {
	uint8_t command;
	uint8_t errorCode;
	uint32_t size;
};

struct TableInfo {
	uint32_t size;
};

struct TableEntryRequest {
	uint8_t command;
	uint16_t tableId;
	uint32_t entryIndex;
};

struct TableEntryResponse {
	uint8_t command;
	uint8_t errorCode;
	uint8_t data[0];
};

struct DateTimeResponse {
	uint8_t command;
	uint8_t errorCode;
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

#pragma pack(pop)

}

#endif
