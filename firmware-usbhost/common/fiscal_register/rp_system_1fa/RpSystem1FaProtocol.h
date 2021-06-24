#ifndef COMMON_FISCALREGISTER_RPSYSTEM1FAPROTOCOL_H
#define COMMON_FISCALREGISTER_RPSYSTEM1FAPROTOCOL_H

#include "utils/include/NetworkProtocol.h"

namespace RpSystem1Fa {

/*
---------------------
Особенности протокола
---------------------
Версия 2.2.11

Структура блока команды и ответа:
	STX <length> <command> <data> <crc>, где
	STX – флаг начала блока команды;
	<length> - длина пакет. В длину не входит STX, <length> и <crc> байты;
	<command> - код команды;
	<crc> - байт контрольной суммы

В варианте ETHERNET не используются байты STX и <crc>, т.к. всю «заботу»
об целостности принимаемых и передаваемых пакетов берет на себя стек TCP/IP.
Схема обмена так же не использует подтверждения ACK, NAK, ENQ.
Хост должен установить соединение с устройством на 3333 порту, отправить
в порт сообщение и ожидать ответа от устройства достаточно времени,
что бы устройство успело проделать работу определяемую командой.
После получения ответа хост должен закрыть соединение.

-------------------------
Дамп проверки связи с ККТ
-------------------------
*/

#define RPSYSTEM1FA_PACKET_MAX_SIZE		100 // уточнить
#define RPSYSTEM1FA_CONFIRM_TIMEOUT		1000
#define RPSYSTEM1FA_RESPONSE_TIMEOUT	10000 // время выполнения команды
#define RPSYSTEM1FA_DELIVER_TIMEOUT		10

enum Control {
	Control_STX  = 0x02,
	Control_ENQ  = 0x05,
	Control_ACK  = 0x06,
	Control_NAK  = 0x15,
};

enum Command {
	Command_Status			= 0x11, // Запрос состояиня ККТ
	Command_ShiftOpen		= 0xE0, // Открыть смену
	Command_CheckOpen		= 0x8D, // Открыть чек
	Command_CheckAddSale	= 0x80, // Добавить позицию
	Command_CheckClose		= 0x85, // Закрыть чек
	Command_CheckCancel		= 0x88, // Отмена чека
	Command_ShiftClose		= 0x41, // Закрыть смену
};

enum Error {
	Error_OK				= 0,  // Нет ошибок
	Error_Shift24More		= 22, // Продолжительность смены долее 24 часов
	Error_ShiftClosed		= 61, // Смена не открыта
	Error_DocumentOpened	= 74, // Документ уже открыт
};

enum FrState {
	FrState_Idle				 = 0x00, // Состояние устройства после старта (фактически можно рассматривать это состояние как 0x04 «Смена закрыта»)
	FrState_ShiftOpened			 = 0x02, // Смена открыта
	FrState_Shift24More			 = 0x03, // Смена открыта более 24 часов
	FrState_ShiftClosed			 = 0x04, // Смена закрыта
	FrState_WaitDateConfirm		 = 0x06, // Ожидает подтверждения даты
	FrState_DocumentIn			 = 0x08, // Открыт документ прихода
	FrState_DocumentOut			 = 0x18, // Открыт документ расхода
	FrState_DocumentReturnIn	 = 0x28, // Открыт документ возврата прихода
	FrState_DocumentReturnOut	 = 0x38, // Открыт документ возврата расхода
	FrState_FatalError			 = 0xFF, // Фатальная ошибка устройства
};

enum ModeFlag {
	ModeFlag_Encoding	 = 0x01, // Шифрование
	ModeFlag_Autonomous	 = 0x02, // Автономный режим
	ModeFlag_Automatic	 = 0x04, // Автоматический режим
	ModeFlag_Service	 = 0x08, // Применение в сфере услуг
	ModeFlag_BSO		 = 0x10, // Режим БСО
	ModeFlag_Internet	 = 0x20, // Для использования при расчётах в сети Интернет
};

enum TaxFlag {
	TaxFlag_OSNO	 = 0x01, // Общая
	TaxFlag_USNO6	 = 0x02, // Упрощённая доход
	TaxFlag_USNO15	 = 0x04, // Упрощённая доход минус расход
	TaxFlag_ENVD	 = 0x08, // Единый налог на вменённый доход
	TaxFlag_ESHN	 = 0x10, // Единый сельскохозяйственный налог
	TaxFlag_PSNO	 = 0x20, // Патентная система налогообложения
};

enum LifePhase {
	LifePhase_Setup			 = 0, // Настройка
	LifePhase_ReadyToFiscal	 = 1, // Готовность к фискализации
	LifePhase_Fiscal		 = 3, // Фискальный режим
	LifePhase_PostFiscal	 = 7, // Пост-фискальный режим, идёт передача ФД в ОФД
	LifePhase_FnReading		 = 15, // Чтение данных из архива ФН
};

enum PrinterState {
	PrinterState_Idle		 = 0, // Принтер в оффлайне (может нет ошибок?)
	PrinterState_PaperOff	 = 1, // Закончилась бумага
	PrinterState_PaperLess	 = 2, // Бумага скоро закончится
	PrinterState_Open		 = 3, // Открыта крышка принтера
	PrinterState_Error1		 = 4, // Восстановимая ошибка принтера
	PrinterState_Error2		 = 5, // Невосстановимая ошибка принтера
	PrinterState_Error3		 = 6, // Принтер прислал неправильный ответ
};

enum FnFlag {
	FnFlag_3DaysLeft	 = 0x01, // Срочная замена КС (до окончания срока действия 3 дня)
	FnFlag_30DaysLeft	 = 0x02, // Исчерпание ресурса КС (до окончания срока действия 30 дней)
	FnFlag_MemoryEnds	 = 0x04, // Переполнение памяти ФН (Архив ФН заполнен на 90 %)
	FnFlag_Timeout		 = 0x08, // Превышено время ожидания ответа ОФД
	FnFlag_CriticalError = 0x80, // Критическая ошибка ФН
};

enum DocumentType {
	DocumentType_In			 = 0, // Приход
	DocumentType_Out		 = 1, // Расход
	DocumentType_ReturnIn	 = 2, // Возврат прихода
	DocumentType_ReturnOut	 = 3, // Возврат расхода
};

enum PaymentType {
	PaymentType_FullPayBeforeTake			= 1, // Полная предварительная оплата до момента передачи предмета расчёта
	PaymentType_PartialPayBeforeTake		= 2, // Частичная предварительная оплата до момента передачи предмета расчёта
	PaymentType_Prepayment					= 3, // Аванс
	PaymentType_FullPayAndTake				= 4, // Полная оплата, в том числе с учётом аванса (предварительной оплаты) в момент передачи предмета расчёта
	PaymentType_PartialPayAndTakeInCredit	= 5, // Частичная оплата предмета расчёта в момент его передачи с последующей оплатой в кредит
	PaymentType_TakeInCredit				= 6, // Передача предмета расчёта без его оплаты в момент его передачи с последующей оплатой в кредит
	PaymentType_CreditPayment				= 7, // Оплата предмета расчёта после его передачи с оплатой в кредит (оплата кредита)
};

/*
enum TaxType {
	TaxType_OSNO	 = 1, // Общая
	TaxType_USNO6	 = 2, // Упрощённая доход
	TaxType_USNO15	 = 3, // Упрощённая доход минус расход
	TaxType_ENVD	 = 4, // Единый налог на вменённый доход
	TaxType_ESHN	 = 5, // Единый сельскохозяйственный налог
	TaxType_PSNO	 = 6, // Патентная система налогообложения
};*/

#pragma pack(push,1)
/*
 * Стандартная команда без дополнительных параметров.
 */
struct Request {
	uint8_t command;
	BEUint4 password;
};

struct Response {
	uint8_t command;
	uint8_t errorCode;  // Код ошибки
};

struct StatusResponse {
	uint8_t command;
	uint8_t errorCode;
	uint8_t softwareVersion[2];	// версия ПО
	BEUint2 softwareBuild;		// номер сборки ПО
	uint8_t softwareDate[3];	// дата сборки ПО
	uint8_t reserved1[5];
	uint8_t state;				// текущее состояние ФР (смотри FrState)
	uint8_t lifePhase;			// смотри LifePhase
	uint8_t printerLastState;	// состояние принтера после печати последнего документа
	uint8_t fnDate[3];			// срок действия ФН
	uint8_t fnFlags;			// флаги состояния ФН (смотри FnFlag)
	uint8_t printerState;		// текущее состояние принтера (смотри PrinterState)
	uint8_t reserved2[2];
	uint8_t date[3];			// текущая дата
	uint8_t time[3];			// текущее время
	uint8_t reserved3[37];
};

struct CheckOpenRequest {
	uint8_t command;
	BEUint4 password;
	uint8_t documentType;
};

struct CheckAddRequest {
	uint8_t command;
	BEUint4 password;
	BEUint5 number;			// Количество
	BEUint5 price;			// Цена
	uint8_t paymentType;	// Признак способа расчёта (смотрите PaymentType)
	uint8_t taxType;		// Код налога WTF?
	uint8_t reserved1[3];
	uint8_t name[0];		// Описание, длина не более ширины строки печатающего устройства

	void setName(const char *str, uint16_t strLen) {
		uint16_t i = 0;
		for(; i < strLen && str[i] != '\0'; i++) {
			name[i] = str[i];
		}
	}
};

struct CheckCloseRequest {
	uint8_t command;
	BEUint4 password;
	BEUint5 cash;			// Сумма наличными
	BEUint5 cashless1;		// Сумма электронными 1-го типа
	BEUint5 cashless2;		// Сумма электронными 2-го типа
	BEUint5 cashless3;		// Сумма электронными 3-го типа
	uint8_t reserved1[6];
	uint8_t text[0];		// Текст, длина не более ширины строки печатающего устройства

	void setText(const char *str, uint16_t strLen) {
		uint16_t i = 0;
		for(; i < strLen && str[i] != '\0'; i++) {
			text[i] = str[i];
		}
	}
};

#pragma pack(pop)

}

#endif
