#ifndef COMMON_ATOLPROTOCOL_H
#define COMMON_ATOLPROTOCOL_H

#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "tcpip/include/TcpIp.h"

#include <stdint.h>

namespace Atol {
/*
---------------------
Особенности протокола
---------------------
Структура блока команды и ответа:
	STX <data[N]> ETX <CRC>, где
	STX – флаг начала блока команды или ответа
	<data[N]> – посылаемые данные (N байт)
	ETX – флаг конца блока команды или ответа
	<CRC> - байт контрольной суммы

Кодопрозрачность:
	Байты данных, равные DLE и ETX, передаются как последовательность двух байт: 10h как <DLE DLE>, 03h как <DLE ETX>.
	Аналог \ в формате printf(): \ = "\\" и перевод коретки = "\n"

-------------------------
Дамп проверки связи с ККТ
-------------------------
xFE;=STX
x01;x00;=LEN
x06;=LD
xC4;=Buffer_Abort
x58;=CRC

xFE;=STX
x06;x00;=LEN
x07;=LD
xC1;=Buffer_Add
x01;=BufferFlag_NeedResult
x02;=TID(02)
x00;x00;=Password(0)
xA5;=Command(Прочитать данные для ККТ)
x8D;=CRC

xFE;=STX
x02;x00;=LEN
x08;=LD
xC3;=BufferCommand_Req
x02;=TID(02)
xFC;=CRC

xFE;=STX
x02;x00;=LEN
x09;=LD
xC3;=BufferCommand_Req
x02;=TID(02)
xBA;=CRC

xFE;=STX
x02;x00;=LEN
x0A;=LD
xC3;=BufferCommand_Req
x02;=TID(02)
x70;=CRC
*/

#define ATOL_MANUFACTURER	"ATL"
#define ATOL_MODEL			"Казначей ФА"
#define ATOL_PACKET_MAX_SIZE		120   // todo: уточнить
#define ATOL_PACKET_ID_MIN_NUMBER	0x01
#define ATOL_PACKET_ID_MAX_NUMBER	0xDE
#define ATOL_PACKET_TIMEOUT         5000
#define ATOL_TASK_ID_MIN_NUMBER		0x01
#define ATOL_TASK_ID_MAX_NUMBER		0xDE
#define ATOL_TASK_ASYNC_TIMEOUT		15000 // увеличено так как не успевало печатать чек и спамило ошибки
#define ATOL_TASK_INIT_DELAY		500
#define ATOL_TASK_RECONNECT_TIMEOUT 2000
#define ATOL_TASK_TRY_NUMBER		5
#define ATOL_KKT_TRY_NUMBER			3

enum Control {
	Control_STX  = 0xFE,
	Control_ESC  = 0xFD,
	Control_TSTX = 0xEE,
	Control_TESC = 0xED,
};

enum Error {
	Error_OK			= 0x00,
	Error_Overflow      = 0xB1,
	Error_AlreadyExists = 0xB2,
	Error_NotFound      = 0xB3,
	Error_IllegalValue  = 0xB4,
};

enum TaskCommand {
	TaskCommand_Add    = 0xC1,
	TaskCommand_Ack    = 0xC2,
	TaskCommand_Req    = 0xC3,
	TaskCommand_Abort  = 0xC4,
	TaskCommand_AckAdd = 0xC5,
};

enum TaskFlag {
	TaskFlag_NeedResult    = 0x01,
	TaskFlag_IgnoreError   = 0x02,
	TaskFlag_WaitAsyncData = 0x04,
};

enum TaskStatus {
	TaskStatus_Pending     = 0xA1,
	TaskStatus_InProgress  = 0xA2,
	TaskStatus_Result      = 0xA3,
	TaskStatus_Error       = 0xA4,
	TaskStatus_Stopped     = 0xA5,
	TaskStatus_AsyncResult = 0xA6,
	TaskStatus_AsyncError  = 0xA7,
	TaskStatus_Waiting     = 0xA8,
};

enum Command {
	Command_DeviceInfo			= 0xA5, // Запрос информации по устройству
	Command_DeviceStatus		= 0xA4, // Запрос состояние устройства
	Command_Status				= 0x3F, // Запрос состояния ККТ
	Command_PrinterStatus		= 0x45, // Запрос состояния принтера
	Command_ModeOpen			= 0x56, // Вход в режим
	Command_ShiftOpen			= 0x9A, // Открыть смену
	Command_CheckOpen			= 0x92, // Открыть чек
	Command_CheckAddSale		= 0xE6, // -Добавить позицию (ФН 1.00)
	Command_FN105CheckAddStart	= 0xEA,	// Начать формирование позиции (ФН 1.05)
	Command_FN105CheckAddEnd	= 0xEB,	// Завершить формирование позиции (ФН 1.05)
	Command_CheckClose			= 0x4A, // Закрыть чек
	Command_CheckReset			= 0x59, // Отмена чека
	Command_ModeClose			= 0x48, // Выход из режима
	Command_ShiftClose			= 0x5A, // Закрыть смену
	Command_RegisterRead		= 0x91, // Прочитать регистр
	Command_FiscalStorage		= 0xA4, // Работа с фискальным накопителем
};

enum CommandError {
	CommandError_OK					= 0x00, // Нет ошибок
	CommandError_WrongMode			= 0x66, // Команда не реализуется в данном режиме ККТ
	CommandError_PrinterNoPaper		= 0x67, // В принтере нет бумаги
	CommandError_PrinterNotFound	= 0x68, // Нет связи с принтером чеков
	CommandError_ShiftMore24		= 0x88, // Смена больше 24 часов
	CommandError_WrongPassword		= 0x8C, // Неправильный пароль
	CommandError_ShiftAlreadyOpened	= 0x9C, // Смена открыта, операция невозможна
	CommandError_CheckAlreadyOpened	= 0x9B, // Чек открыт – операция невозможна
};

enum FSSubcommand {
	FSSubcommand_FiscalNumber = 0x31, // Запрос номера ФН
	FSSubcommand_DocSize = 0x45, // Запрос размера документа по номеру
	FSSubcommand_DocData = 0x46, // Запрос данных документа после запроса длины
};

enum Mode { // стр. 95, Таблица состояний
	Mode_Mask           = 0x0F,
	Mode_Idle			= 0x00,
	Mode_Sale			= 0x01,
	Mode_ReportCounts	= 0x02,
	Mode_ReportZ		= 0x03,
	Mode_Programming	= 0x04,
	Mode_EnterWTF		= 0x05,
	Mode_FiscalStorage	= 0x06,
	Mode_Addition		= 0x07,
};

enum ModeFlag {
	ModeFlag_KKTRegisterd		= 0x01, // ККТ зарегистрирована (0 - нет, 1 - да)
	ModeFlag_ShiftOpened		= 0x02, // смена открыта (0 - нет, 1 - да)
	ModeFlag_PaperAvailable		= 0x08,	// датчик ЧЛ (0 - нет бумаги, 1 - есть бумага)
	ModeFlag_FiscalActivated	= 0x40,	// состояние ФН: 0 – не активизирован, 1 – активизирован)
	ModeFlag_BattaryAvailable	= 0x80,	// батарейка установлена (0 - нет, 1 - да)
};

enum Register {
	Register_LastSale		 = 0x33,
	Register_LastFiscalDoc	 = 0x34,
};

#pragma pack(push,1)
/*
 * Пакет транспортного уровня.
 */
struct PacketHeader {
	uint8_t stx;
	uint8_t len[2];
	uint8_t id;
	uint8_t data[0];

	void set(uint8_t packetId, uint16_t dataLen) {
		this->stx = Control_STX;
		this->len[0] = dataLen & 0x7F;
		this->len[1] = dataLen >> 7;
		this->id = packetId;
	}

	uint16_t getDataLen() {
		return ((len[0] & 0x7F) | len[1] << 7);
	}
};

struct TaskResponse {
	uint8_t result;
	uint8_t data[0];
};

struct TaskAddHeader {
	uint8_t command;
	uint8_t flags;
	uint8_t tid;
	uint8_t data[0];
};

struct TaskStoppedResponse {
	uint8_t command;
	uint8_t tid;
};

struct TaskAsyncResult {
	uint8_t result;
	uint8_t tid;
	uint8_t data[0];
};

struct TaskReqRequest {
	uint8_t command;
	uint8_t tid;
};

struct TaskAbortHeader {
	uint8_t command;
	uint8_t data[0];
};

/*
 * Стандартная команда без дополнительных параметров.
 */
struct Request {
	LEUbcd2 password;
	uint8_t command;
};

struct Response {
	uint8_t resultCode; // 0х55
	uint8_t errorCode;  // Код ошибки
	uint8_t rezerved2;  // Все время 0
};

struct SubcommandRequest {
	LEUbcd2 password;
	uint8_t command;
	uint8_t subcommand;
};

struct ModeOpenRequest {
	LEUbcd2 password1;
	uint8_t command;
	uint8_t mode;
	LEUbcd4 password2;
};

/*
x00;<Код_ошибки (1)>
x01;<Версия_протокола (1)>
x01;<Тип (1)>
x4C;<Модель (1)> что за модель?
x10;x00;<режим (2)> что за режим?
x03;x00;x00;x21;x95;<Версия_устройства (5)> 1-major, 1-minor, 3-build (BCD)
x8A;xA0;xA7;xAD;xA0;xE7;xA5;xA9;x20;x94;x80;<Название (N)> "Казначей ФА" CP866
*/
struct DeviceInfoResponse {
	uint8_t errorCode;			// код ошибки
	uint8_t protocolVersion;    // версия протокола
	uint8_t deviceType;         // тип устройства
	uint8_t deviceId;           // идентификатор устройства
	uint8_t deviceMode[2];      // режим работы (флаги)
	Ubcd1   deviceVersionMajor; // версия устройства - первая цифра (BCD)
	Ubcd1   deviceVersionMinor; // версия устройства - вторая цифра (BCD)
	Ubcd1   deviceVersionLang;  // версия устройства - язык (BCD)
	Ubcd1   deviceVersionBuild; // версия устройства - сборка (BCD)
	uint8_t deviceName[0];      // название (CP866)
};

struct PrinterStatusResponse {
	uint8_t resultCode;
	uint8_t deviceMode;
	uint8_t printerMode;
};

struct StatusResponse {
	uint8_t resultCode;         // Всегда 0x44h
	Ubcd1   cashier;            // Номер кассира (BCD1)
	uint8_t number;             // Номер в зале
	Ubcd1   dataYear;           // Год (0-99)
	Ubcd1   dataMonth;          // Месяц (1-12)
	Ubcd1   dataDay;            // День (1-31)
	Ubcd1   timeHour;           // Часы (0-24)
	Ubcd1   timeMinute;         // Минуты (0-59)
	Ubcd1   timeSecond;         // Секунды (0-59)
	uint8_t flags;              // Флаги
	LEUbcd4 serialNumber;       // Серийный номер (BCD4, не более 999999)
	uint8_t deviceId;           // Модель устройства (для Казначения равно 76)
	uint8_t deviceVersionMajor; // версия устройства - первая цифра (BCD1)
	uint8_t deviceVersionMinor; // версия устройства - вторая цифра (BCD1)
	uint8_t deviceMode;         // режим работы (флаги)
	LEUbcd2 checkNumber;        // номер чека (BCD2)
	LEUbcd2 shiftNumber;        // номер смены (BCD2)
	uint8_t checkState;			// состояние чека
	LEUint5 checkSum;           // сумма чека
	uint8_t decimalPoint;       // десятичая точка (0-4)
	uint8_t interfaceId;        // номер интерфейса (1-RS232, 4-USB, 5-Bluetooth/Wifi, 6-Ethernet)

	uint8_t getDeviceMode() { return deviceMode & Mode_Mask; }
	uint8_t getDeviceSubMode() { return (deviceMode >> 4) & 0x0F; }
};

enum ShiftOpenFlag {
	ShiftOpenFlag_Test     = 0x01, // режим тестирования
};

struct ShiftOpenRequest {
	LEUbcd2 password;
	uint8_t command;
	uint8_t flags;
	uint8_t text[0];
};

enum CheckOpenFlag {
	CheckOpenFlag_Test     = 0x01, // режим тестирования
	CheckOpenFlag_NotPrint = 0x04, // не печатать чек
};

enum CheckOpenType {
	CheckType_Sale		 = 1,
	CheckType_SaleReturn = 2,
	CheckType_Buy		 = 4,
	CheckType_BuyReturn	 = 5,
};

struct CheckOpenRequest {
	LEUbcd2 password;
	uint8_t command;
	uint8_t flags;
	uint8_t checkType;
};

struct CheckAddRequest {
	LEUbcd2 password;
	uint8_t command;		// 0xE6
	uint8_t flags;			// Флаги
	uint8_t name[64];		// Название (CP866)
	LEUbcd6 price;			// Цена (BCD6)
	LEUbcd5 number;			// Количество (BCD5)
	uint8_t discountType;	// Тип процент/число
	uint8_t discountSign;	// Знак скидка/наценка
	LEUbcd6 discountSize;	// азмер (BCD6)
	uint8_t tax;			// Налоговая ставка
	Ubcd1   section;		// Секция в которую записывается оплата (BCD1)
	uint8_t text[16];		// Текст (ASCII) todo: выяснить зачем этот текст - на чеке не выводится
	uint8_t reserved;

	void setName(const char *str, uint16_t strLen = 64) {
		uint16_t i = 0;
		for(; i < sizeof(name) && i < strLen && str[i] != '\0'; i++) {
			name[i] = str[i];
		}
		for(; i < sizeof(name); i++) {
			name[i] = '\0';
		}
		convertWin1251ToCp866(name, sizeof(name));
	}

	void setText(const char *str, uint16_t strLen = 16) {
		uint16_t i = 0;
		for(; i < sizeof(text) && i < strLen && str[i] != '\0'; i++) {
			text[i] = str[i];
		}
		for(; i < sizeof(text); i++) {
			text[i] = '\0';
		}
		convertWin1251ToCp866(text, sizeof(text));
	}
};

struct FN105CheckAddStartRequest {
	LEUbcd2 password;
	uint8_t command;		// 0xEA
	uint8_t flags;			// Флаги
	uint8_t param;
	uint8_t reserved;
};

struct FN105CheckAddEndRequest {
	LEUbcd2 password;
	uint8_t command;		// 0xEB
	uint8_t flags;			// Флаги
	LEUbcd7 price;			// Цена (BCD7)
	LEUbcd5 number;			// Количество (BCD5)
	LEUbcd7 value;			// Фактическая цена (BCD7)
	uint8_t taxRate;		// Ставка налога
	LEUbcd7 taxSum;			// Сумма налога
	Ubcd1   section;		// Секция в которую записывается оплата (BCD1)
	Ubcd1   saleSubject;	// Предмет расчета (тег 1212)
	Ubcd1   saleMethod;		// Способ расчета
	uint8_t discountSign;	// Знак скидки
	LEUbcd7 discountSize;	// Размер скидки (BCD7)
	uint8_t reserved[2];
	uint8_t name[0];		// Название (CP866). Передается фактическое название

	uint16_t setName(const char *str, uint16_t strLen = FN105_CHECK_POS_NAME_MAX_SIZE) {
		uint16_t i = 0;
		for(; i < FN105_CHECK_POS_NAME_MAX_SIZE && i < strLen && str[i] != '\0'; i++) {
			name[i] = str[i];
		}
		convertWin1251ToCp866(name, i);
		return i;
	}
};

struct CheckCloseRequest {
	LEUbcd2 password;
	uint8_t command;		// 0x4A
	uint8_t flags;			// Флаги
	Ubcd1	paymentType;	// Способ оплаты
	LEUbcd5 enterSum;		// Внесенная сумма (BCD5)
};

struct RegisterReadRequest {
	LEUbcd2 password;
	uint8_t command;
	uint8_t reg;
	uint8_t param1;
	uint8_t param2;
};

struct RegisterReadResponse {
	uint8_t resultCode; // 0х55
	uint8_t errorCode;  // Код ошибки
	LEUbcd5 fdNumber;
	uint8_t type;
	LEUbcd7 total;
	LEUbcd5 datetime;
	LEUbcd5 fiscalSign;
	uint8_t reserved[3];
};

struct FSDocSizeRequest {
	LEUbcd2 password;
	uint8_t command;
	uint8_t subcommand;
	BEUint4 docNumber;
};

struct FSDocSizeResponse {
	uint8_t resultCode; // 0х55
	uint8_t errorCode;  // Код ошибки
	BEUint2 docType;
	BEUint2 docSize;
};

struct FSDocDataResponse {
	uint8_t resultCode; // 0х55
	uint8_t errorCode;  // Код ошибки
	uint8_t data[0];
};

#pragma pack(pop)

class TaskLayerObserver {
public:
	enum Error {
		Error_OK = 0,
		Error_ConnectFailed,
		Error_RemoteClose,
		Error_TaskFailed,
		Error_AbortFailed,
		Error_PacketTimeout,
		Error_PacketSendFailed,
		Error_PacketRecvFailed,
		Error_PacketWrongSize,
		Error_UnknownError,
	};

	virtual ~TaskLayerObserver() {}
	virtual void procRecvData(const uint8_t *data, const uint16_t len) = 0;
	virtual void procError(Error error) = 0;
};

class TaskLayerInterface {
public:
	virtual ~TaskLayerInterface() {}
	virtual void setObserver(TaskLayerObserver *observer) = 0;
	virtual bool connect(const char *domainname, uint16_t port, TcpIp::Mode mode) = 0;
	virtual bool sendRequest(const uint8_t *data, const uint16_t dataLen) = 0;
	virtual bool disconnect() = 0;
};

class PacketLayerObserver {
public:
	enum Error {
		Error_OK = 0,
		Error_ConnectFailed,
		Error_RemoteClose,
		Error_SendFailed,
		Error_RecvFailed,
		Error_RecvTimeout,
	};

	virtual ~PacketLayerObserver() {}
	virtual void procRecvData(uint8_t packetId, const uint8_t *data, const uint16_t len) = 0;
	virtual void procError(Error error) = 0;
};

class PacketLayerInterface {
public:
	virtual ~PacketLayerInterface() {}
	virtual void setObserver(PacketLayerObserver *observer) = 0;
	virtual bool connect(const char *domainname, uint16_t port, TcpIp::Mode mode) = 0;
	virtual bool sendPacket(const uint8_t *data, const uint16_t dataLen) = 0;
	virtual bool disconnect() = 0;
};

/*
 * Полином: x8+x5+x4+1 / 0x31 / “CRC-8-Dallas/Maxim”
 */
class Crc {
public:
	void start() { crc = CRC8INIT; }
	void add(uint8_t byte) {
		crc ^= byte;
		for(uint8_t i = 0; i < 8; i++) {
			if(crc & 0x80) { crc = (crc << 1) ^ CRC8POLY; }
			else { crc <<= 1; }
		}
	}
	uint8_t getCrc() { return crc; }
private:
	static const uint8_t CRC8INIT = 0xFF;
	static const uint8_t CRC8POLY = 0x31;
	uint8_t crc;
};

}

#endif
