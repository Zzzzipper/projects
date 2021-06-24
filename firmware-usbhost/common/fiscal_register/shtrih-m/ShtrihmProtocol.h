#ifndef SHTRIHMPROTOCOL_H
#define SHTRIHMPROTOCOL_H

#include "utils/include/NetworkProtocol.h"

/*
---------------------
Особенности протокола
---------------------
Разрядность денежных величин:
  Все суммы в данном разделе – целые величины, указанные в «МДЕ».
  МДЕ – минимальная денежная единица. С 01.01.1998 в Российской Федерации 1 МДЕ равна
  1 копейке (до 01.01.1998 1 МДЕ была равна 1 рублю).
Формат передачи значений:
  Все числовые величины передаются в двоичном формате, если не указано другое.
  Первым передается самый младший байт, последним самый старший байт.
  При передаче даты (3 байта) сначала передаётся число (1 байт – ДД),
  затем месяц (2 байта – ММ), и последним – год (1 байт – ГГ).
  При передаче времени (3 байта) первым байтом передаются часы (1 байт – ЧЧ),
  затем минуты (2 байта – ММ), и последними передаются секунды (1 байт – СС).
Ответы и коды ошибок:
  Ответное сообщение содержит корректную информацию, если код ошибки
  (второй байт в ответном сообщении) 0.
  Если код ошибки не 0, передается только код команды и код ошибки – 2 байта.
 */

#define SHM_ENQ_ANSWER_TIMER		1000
#define SHM_ENQ_TRY_MAX_NUMBER		3
#define SHM_REQUEST_CONFIRM_TIMER	1000
#define SHM_PACKET_MAX_SIZE			256
#define SHM_WAIT_STX_TIMER			5000 // разработчик сообщил, что ФР может задумываться до 5 секунд
#define SHM_WAIT_LENGHT_TIMER		1000
#define SHM_WAIT_DATA_TIMER			1000
#define SHM_ANSWER_TIMER			50
#define SHM_SKIP_DATA_TIMER			300

enum ShmControl {
	ShmControl_STX = 0x02,
	ShmControl_ENQ = 0x05,
	ShmControl_ACK = 0x06,
	ShmControl_NAK = 0x15,
	ShmControl_UNKNOWN = 0xFF,
};

enum ShmCommand {
	ShmCommand_ShortStatus		= 0x10,
	ShmCommand_PrintContinue	= 0xB0,
	ShmCommand_OpenShift		= 0xE0,
	ShmCommand_CheckOpen		= 0x8D,
	ShmCommand_CheckAddSale		= 0x80,
	ShmCommand_CheckClose		= 0x85,
	ShmCommand_CheckReset		= 0x88,
	ShmCommand_ShiftClose		= 0x41,
};

enum ShmError {
	ShmError_OK				= 0x00, // Нет ошибок
	ShmError_ShiftClosed	= 0x16,	// Нет открытой смены
	ShmError_Shift24More	= 0x4E, // Сессия открыта больше 24 часов
	ShmError_Printing		= 0x50, // Печать предыдущего документа
	ShmError_WaitPrint		= 0x58, // Ожидание команды продолжения печати (ShmCommand_PrintContinue)
};

enum ShmDocumentType {
	ShmDocumentType_Sale		 = 0,
	ShmDocumentType_Buy			 = 1,
	ShmDocumentType_SaleReturn	 = 2,
	ShmDocumentType_BuyReturn	 = 3,
};

enum ShmKKTMode {
	ShmKKTMode_Mask				 = 0x0F,
	ShmKKTMode_DataOut			 = 1,	// Выдача данных
	ShmKKTMode_ShiftOpened24Less = 2,	// Открытая смена, 24 часа не кончились
	ShmKKTMode_ShiftOpened24More = 3,	// Открытая смена, 24 часа кончились
	ShmKKTMode_ShiftClosed		 = 4,	// Закрытая смена
	ShmKKTMode_BlockedByPassword = 5,	// Блокировка по неправильному паролю налогового инспектора
	ShmKKTMode_WaitDateConfirm	 = 6,	// Ожидание подтверждения ввода даты
	ShmKKTMode_DecimalPlaces	 = 7,	// Разрешение изменения положения десятичной точки1
	ShmKKTMode_Document			 = 8,	// Открытый документ
	ShmKKTMode_TechnicalZero	 = 9,	// Режим разрешения технологического обнуления. В этот режим ККМ переходит по включению питания, если некорректна информация в энергонезависимом ОЗУ ККМ.
	ShmKKTMode_Test				 = 10,	// Тестовый прогон
	ShmKKTMode_PrintFiscalReport = 11,	// Печать полного фискального отчета
	ShmKKTMode_PrintFNReport	 = 12,	// Печать отчёта ЭКЛЗ
	ShmKKTMode_Document1		 = 13,	// Работа с фискальным подкладным документом1
	ShmKKTMode_PrintDocument1	 = 14,	// Печать подкладного документа1
	ShmKKTMode_Document1Complete = 15,	// Фискальный подкладной документ сформирован1
};

#pragma pack(push,1)
/*
Стандартная команда без дополнительных параметров.
 */
struct ShmRequest {
	uint8_t command;
	BEUint4 password;
};

/*
Ответ на любую команду в случае ошибки или ответ без дополнительных параметров.
 */
struct ShmResponse {
	uint8_t command;
	uint8_t errorCode;
};

struct ShmShortStatusResponse {
	uint8_t command;
	uint8_t errorCode;
	uint8_t operatorNumber;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t mode;
	uint8_t submode;
	uint8_t operNumberLow;
	uint8_t reserveVoltage;
	uint8_t voltage;
	uint8_t frErrorCode;
	uint8_t fnErrorCode;
	uint8_t operNumberHigh;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;

	uint16_t getOperNumber() {
		uint16_t value = (operNumberHigh << 8) | operNumberLow;
		return value;
	}
};

struct ShmPrintContinueResponse {
	uint8_t command;
	uint8_t errorCode;
	uint8_t operatorNumber;
};

struct ShmCheckAddSaleRequest {
	uint8_t command;
	BEUint4 password;
	BEUint5 number;
	BEUint5 price;
	uint8_t department;
	BEUint4 tax;
	uint8_t name[40];

	void setName(const char *value) {
		uint8_t nameMaxLen = 39;
		uint8_t nameBufSize = 40;
		uint8_t i = 0;
		for(; i < nameMaxLen; i++) {
			name[i] = value[i];
			if(value[i] == '\0') {
				break;
			}
		}
		for(; i < nameBufSize; i++) {
			name[i] = '\0';
		}
	}
};

struct ShmCheckCloseRequest {
	uint8_t command;   // 85H. Длина сообщения: 71 или 40+Y1,2 байт.
	BEUint4 password;  // Пароль оператора (4 байта)
	BEUint5 cash;      // Сумма наличных (5 байт) 0000000000…9999999999
	BEUint5 value2;    // Сумма типа оплаты 2 (5 байт) 0000000000…9999999999
	BEUint5 value3;    // Сумма типа оплаты 3 (5 байт) 0000000000…9999999999
	BEUint5 value4;    // Сумма типа оплаты 4 (5 байт) 0000000000…9999999999
	BEUint2 discont;   // Скидка/Надбавка(в случае отрицательного значения) в % на чек от 0 до 99,99 % (2 байта со знаком) -9999…9999
	uint8_t tax1;      // Налог 1 (1 байт) «0» – нет, «1»…«4» – налоговая группа
	uint8_t tax2;      // Налог 2 (1 байт) «0» – нет, «1»…«4» – налоговая группа
	uint8_t tax3;      // Налог 3 (1 байт) «0» – нет, «1»…«4» – налоговая группа
	uint8_t tax4;      // Налог 4 (1 байт) «0» – нет, «1»…«4» – налоговая группа
	uint8_t name[40];  // Текст3,4,5,6 (40 или до Y1,2 байт)

	void setName(const char *value) {
		uint8_t nameMaxLen = 39;
		uint8_t nameBufSize = 40;
		uint8_t i = 0;
		for(; i < nameMaxLen; i++) {
			name[i] = value[i];
			if(value[i] == '\0') {
				break;
			}
		}
		for(; i < nameBufSize; i++) {
			name[i] = '\0';
		}
	}
};

struct ShmCheckCloseResponse {
	uint8_t command;
	uint8_t errorCode;
	uint8_t operatorNumber;
	uint8_t change;
	char url[0];
};
#pragma pack(pop)

class ShtrihmCrc {
public:
	void start(uint8_t byte) { crc = byte; }
	void add(uint8_t byte) { crc = crc ^ byte; }
	uint8_t getCrc() { return crc; }
	static uint8_t calc(const Buffer *data) {
		uint8_t dataLen = data->getLen();
		ShtrihmCrc crc;
		crc.start(dataLen);
		for(uint8_t i = 0; i < dataLen; i++) {
			crc.add((*data)[i]);
		}
		return crc.getCrc();
	}

private:
	uint8_t crc;
};

#endif
