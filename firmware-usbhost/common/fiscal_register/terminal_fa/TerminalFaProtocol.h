#ifndef COMMON_TERMINALFA_PROTOCOL_H
#define COMMON_TERMINALFA_PROTOCOL_H

#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Buffer.h"
#include "fiscal_storage/include/FiscalStorage.h"

#include <stdint.h>

namespace TerminalFa {

#define TERMINALFA_PACKET_MAX_SIZE 120   // todo: уточнить
#define TERMINALFA_RESPONSE_TIMER 5000

enum Control {
	Control_FB  = 0xB6,
	Control_SB  = 0x29,
};

enum Command {
	Command_Status				= 0x01,
	Command_FSState				= 0x08,
	Command_ShiftState			= 0x20, // проверка состояния смены
	Command_ShiftOpenStart		= 0x21,
	Command_ShiftOpenFinish		= 0x22,
	Command_ShiftCloseStart		= 0x29,
	Command_ShiftCloseFinish	= 0x2A,
	Command_DocumentReset		= 0x10,
	Command_CheckOpen			= 0x23,
	Command_CheckAdd			= 0x2B,
	Command_CheckPayment		= 0x2D,
	Command_CheckClose			= 0x24,
};

enum Result {
	Result_OK	 = 0,
	Result_Error = 1,
};

enum Error {
	Error_ShiftTooOld	 = 0x16,
};

enum LifePhase {
	LifePhase_NotFiscal	 = 0x01, // готов к фискализации
	LifePhase_Fiscal	 = 0x03, // фискализирован
	LifePhase_PostFiscal = 0x07, // постфискальная фаза
	LifePhase_FNReadging = 0x15, // чтение ФН
};

enum DocType {
	DocType_None				= 0x00, // Нет открытого документа
	DocType_RegistrationReport	= 0x01, // Отчет о регистрации ККТ
	DocType_ShiftOpenReport		= 0x02, // Отчет об открытии смены
	DocType_Check				= 0x04, // Кассовый чек
	DocType_ShiftCloseReport	= 0x08, // Отчет о закрытии смены
};

#pragma pack(push,1)
struct Header {
	uint8_t fb;
	uint8_t sb;
	LEUint2 len;
	uint8_t data[0];
};

struct Request {
	uint8_t command;
};

struct Response {
	uint8_t result;
};

struct ErrorResponse {
	uint8_t result;
	uint8_t code;
};

struct StatusResponse {
	uint8_t result;
	uint8_t serial[12];
	uint8_t datetime[5];
	uint8_t criticalError;
	uint8_t printerState;
	uint8_t fnState;
	uint8_t lifePhase;
};

struct FSStateResponse {
	uint8_t result;
	uint8_t lifePhase;
	uint8_t docType;
	uint8_t docData;
	uint8_t shiftState;
	uint8_t flags;
	uint8_t docDatetime[5];
	uint8_t fsNumber[16];
	LEUint4 docNumber;
};

struct ShiftStateResponse {
	uint8_t result;
	uint8_t shiftState;
	LEUint2 shiftNumber;
	LEUint2 checkNumber;
};

struct ShiftOpenStartRequest {
	uint8_t command;
	uint8_t printing;
};

struct CheckCloseRequest {
	uint8_t command;
	uint8_t sign;
	BEUint5 total;
	uint8_t comment[0];
};

#pragma pack(pop)

class PacketLayerInterface {
public:
	enum Error {
		Error_ResponseTimeout	 = 0,
		Error_PacketFormat		 = 1,
	};

	class Observer {
	public:
		virtual void procRecvData(uint8_t *data, uint16_t dataLen) = 0;
		virtual void procRecvError(Error error) = 0;
	};

	virtual void setObserver(Observer *observer) = 0;
	virtual bool sendPacket(Buffer *data) = 0;
};

class Crc {
public:
	Crc() {
		initialValue = 0xffff;
		uint16_t temp, a;
		for(int i = 0; i < tableSize; ++i) {
			temp = 0;
			a = (uint16_t)(i << 8);
			for(int j = 0; j < 8; ++j) {
				if(((temp ^ a) & 0x8000) != 0) {
					temp = (uint16_t)((temp << 1) ^ polynom);
				} else {
					temp <<= 1;
				}
				a <<= 1;
			}
			table[i] = temp;
		}
	}

	void start() {
		crc = initialValue;
	}

	void add(uint8_t byte) {
		crc = (uint16_t)((crc << 8) ^ table[((crc >> 8) ^ (0xff & byte))]);
	}

	void add(uint8_t *data, uint16_t dataLen) {
		for(uint16_t i = 0; i < dataLen; i++) {
			add(data[i]);
		}
	}

	uint16_t getCrc() {
		return crc;
	}

	uint8_t getHighByte() {
		return (uint8_t)(crc >> 8);
	}

	uint8_t getLowByte() {
		return (uint8_t)crc;
	}

private:
	static const uint16_t tableSize = 256;
	static const uint16_t polynom = 0x1021;
	uint16_t table[tableSize];
	uint16_t initialValue;
	uint16_t crc;
};

}

#endif
