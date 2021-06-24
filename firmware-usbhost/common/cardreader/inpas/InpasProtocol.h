#ifndef COMMON_INPAS_PROTOCOL_H
#define COMMON_INPAS_PROTOCOL_H

#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "tcpip/include/TcpIp.h"

#include <stdint.h>

namespace Inpas {
/*
---------------------
Особенности протокола
---------------------
Структура блока команды и ответа:
	STX
	<LENL><LENH> размер данных (только данных без контрольной суммы)
	<data[N]> посылаемые данные (N байт)
	<CRCH><CRCL> контрольная сумма

TLV-формат нестандартный (поле LEN всегда 2 байта)!
Кодировка текста - win1251

Вопросы:
1. как определить версию протокола? никак
3. как отключить оплату/кнопку на пинпаде? никак
4. где описаны таймауты в случае ошибок передачи?
5. как делать перепосылки пакетов, если не было ACK?

-------------------------
Дамп оплаты (ТЕХНОЛОГИЯ РЕВ4, ИНПАС)
-------------------------
>x07;x0D;x0A; //x07\r\n

>x52;x45;x53;x45;x54; x2E;x2E;x2E;x0D;x0A; x4D;x44;x42;x32;x50; x43;x20;x76;x31;x2E;
>x31;x35;x20;x32;x30; x31;x37;x2D;x30;x32; x28;x52;x45;x44;x29; x20;x50;x32;x2A;x0D;
>x0A; // RESET...\r\nMDB2PC v1.15 2017-02(RED) P2*\r\n

>x03;
ETX(x03;)

>x04;
EOT(x04;)

>x04;
EOT(x04;)

// p200 press button
<x02;x17;x00;x00;x05; x00;x32;x30;x30;x30; x30;x04;x03;x00;x36; x34;x33;x56;x06;x00;
<x32;x30;x31;x30;x30; x30;x90;xF9;
STX(x02;)LEN(x17;x00;=23)
FIELD(x00;=credit)LEN(x05;x00;)VALUE(x32;x30;x30;x30;x30;=BCD(20000))
FIELD(x04;=currency)LEN(x03;x00;)VALUE(x36;x34;x33;=643)
FIELD(x56;)LEN(x06;x00;)VALUE(x32;x30;x31;x30;x30;x30;=(decimal-point=02)(scale-factor=01)(mdb-version=00)
CRC(x90;xF9;)

// иногда у Инпаса сбрасывается лимит средств и возвращается некорректное значение
FIELD(x00;=credit)LEN(x0A;x00;)VALUE(x2D;x37;x32;x37;x33;x37;x39;x39;x36;x39;=BCD(20000))
FIELD(x04;=currency)LEN(x03;x00;)VALUE(x36;x34;x33;=643)
FIELD(x56;)LEN(x06;x00;)VALUE(x32;x30;x31;x30;x30;x30;=(decimal-point=02)(scale-factor=01)(mdb-version=00)

>x06;
ACK(x06;)

// payment request
>x02;x15;x00;x00; x08;x00;x30;x30;x30; x30;x30;x31;x30;x30; x04;x03;x00;x36;x34;
>x33;x19;x01;x00;x31; x46;x2E;
STX(x02;)LEN(x15;x00;=21)
FIELD(x00;=credit)LEN(x08;x00;)VALUE(x30;x30;x30;x30;x30;x31;x30;x30;=00000100)
FIELD(x04;=currency)LEN(x03;x00;)VALUE(x36;x34;x33;=643)
FIELD(x19;=operation_code)LEN(x01;x00;)VALUE(x31;=payment)
CRC(x46;x2E;)

<x06;
ACK(x06;)

// payment wait
<x02;x05;x00;x19; x02;x00;x32;x31;xA5; xC2;
STX(x02;)LEN(x05;x00;=5)
FIELD(x19;=)LEN(x02;x00;)VALUE(x32;x31;=wait)
CRC(xA5;xC2;)

>x06;
ACK(x06;)

// payment result (OK)
<x02;x99;x00;x00;x03; x00;x31;x30;x30;x04; x03;x00;x36;x34;x33; x06;x0E;x00;x32;x30;
<x31;x38;x31;x30;x31; x36;x31;x39;x31;x35; x32;x33;x0A;x10;x00; x2A;x2A;x2A;x2A;x2A;
<x2A;x2A;x2A;x2A;x2A; x2A;x2A;x36;x36;x33; x39;x0B;x04;x00;x31; x39;x30;x33;x0D;x06;
<x00;x32;x34;x37;x36; x32;x39;x0E;x0C;x00; x38;x32;x38;x39;x31; x33;x33;x34;x39;x33;
<x37;x33;x0F;x02;x00; x30;x30;x13;x08;x00; xCE;xC4;xCE;xC1;xD0; xC5;xCD;xCE;x15;x0E;
<x00;x32;x30;x31;x38; x31;x30;x31;x36;x31; x39;x31;x35;x32;x33; x17;x02;x00;x2D;x31;
<x19;x01;x00;x31;x1A; x02;x00;x2D;x31;x1B; x08;x00;x30;x30;x32; x36;x36;x32;x38;x36;
<x1C;x09;x00;x31;x31; x31;x31;x31;x31;x31; x31;x31;x27;x01;x00; x31;x8D;x79;
STX(x02;)LEN(x99;x00;)
FIELD(x00;=credit)LEN(x03;x00;)VALUE(x31;x30;x30;=100)
FIELD(x04;=currency)LEN(x03;x00;)VALUE(x36;x34;x33;=643)
FIELD(x06;=host-datetime)LEN(x0E;x00;=14)VALUE(x32;x30;x31;x38;x31;x30; x31;x36; x31;x39; x31;x35; x32;x33;=2018-10-16 19:15:23)
FIELD(x0A;=card-number)LEN(x10;x00;=16)VALUE(x2A;x2A;x2A;x2A; x2A;x2A;x2A;x2A; x2A;x2A;x2A;x2A; x36;x36;x33;x39;=************6639)
FIELD(x0B;=)LEN(x04;x00;=4)VALUE(x31;x39;x30;x33;) WTF?
FIELD(x0D;=auth-code)LEN(x06;x00;=6)VALUE(x32;x34;x37;x36;x32;x39;=247629) WTF?
FIELD(x0E;=ref-number)LEN(x0C;x00;=12)VALUE(x38;x32;x38;x39;x31; x33;x33;x34;x39;x33; x37;x33;=82891333493) WTF?
FIELD(x0F;=host-repsonse-code)LEN(x02;x00;=2)VALUE(x30;x30;=00) WTF?
FIELD(x13;=additional-data)LEN(x08;x00;=8)VALUE(xCE;xC4;xCE;xC1;xD0;xC5;xCD;xCE;=CP1251(ОДОБРЕНО))
FIELD(x15;=terminal-datetime)LEN(x0E;x00;=14)VALUE(x32;x30;x31;x38; x31;x30; x31;x36; x31;x39; x31;x35; x32;x33;=2018-10-16 19:15:23)
FIELD(x17;=transaction-id)LEN(x02;x00;=2)VALUE(x2D;x31;) WTF? ERROR?
FIELD(x19;=operation-code)LEN(x01;x00;=1)VALUE(x31;=payment)
FIELD(x1A;=terminal-transaction-id)LEN(x02;x00;=2)VALUE(x2D;x31;)
FIELD(x1B;=terminal-id)LEN(x08;x00;=8)VALUE(x30;x30;x32;x36;x36;x32;x38;x36;=00266286)
FIELD(x1C;=merchant-id)LEN(x09;x00;=9)VALUE(x31;x31;x31;x31;x31;x31;x31;x31;x31;=111111111)
FIELD(x27;=transaction-result)LEN(x01;x00;=1)VALUE(x31;=1(SUCCESS))
CRC(x8D;x79;)

>x06;
ACK(x06;)

-------------------------
Дамп сетевого соединения
-------------------------
// connection open
FIELD(x19;=OperationCode)LEN(x02;x00;)VALUE(x36;x33;="63"=>user-command)
FIELD(x40;=CommandMode)LEN(x01;x00;)VALUE(x31;="1"=>open)
FIELD(x41;=CommandType)LEN(x02;x00;)VALUE(x31;x36;="16"=>conn)
FIELD(x46;=)LEN(x12;x00;)VALUE(x39;x31;x2E;x31;x30;x37;x2E;x36;x35;x2E;x36;x36;x3B;x31;x39;x37;x39;x3B;="91.107.65.66;1979;")

>x06;
ACK(x06;)

>x19;x02;x00;x36;x33;x41;x02;x00;x31;x36;x43;x01;x00;x30;
FIELD(x19;=OperationCode)LEN(x02;x00;)VALUE(x36;x33;="63"=>user-command)
FIELD(x41;=CommandType)LEN(x02;x00;)VALUE(x31;x36;="16"=>conn)
FIELD(x43;=CommandResult)LEN(x01;x00;)VALUE(x30;="0"=>SUCCESS)

// send data
x19;x02;x00;x36;x33;x40;x01;x00;x30;x41;x02;x00;x31;x37;x46;x93;x00;x50;x4F;x53;x54;x20;x2F;x73;x65;x72;x76;x69;x63;x65;x2D;x73;x65;x6C;x65;x63;x74;x6F;x72;x2F;x20;x48;x54;x54;x50;x2F;x31;x2E;x31;x0D;x0A;x48;x6F;x73;x74;x3A;x20;x39;x31;x2E;x31;x30;x37;x2E;x36;x35;x2E;x36;x36;x3A;x31;x39;x37;x39;x0D;x0A;x43;x6F;x6E;x74;x65;x6E;x74;x2D;x54;x79;x70;x65;x3A;x20;x61;x70;x70;x6C;x69;x63;x61;x74;x69;x6F;x6E;x2F;x6F;x63;x74;x65;x74;x2D;x73;x74;x72;x65;x61;x6D;x0D;x0A;x43;x6F;x6E;x74;x65;x6E;x74;x2D;x4C;x65;x6E;x67;x74;x68;x3A;x20;x33;x34;x31;x0D;x0A;x43;x61;x63;x68;x65;x2D;x43;x6F;x6E;x74;x72;x6F;x6C;x3A;x20;x6E;x6F;x2D;x73;x74;x6F;x72;x65;x0D;x0A;x0D;x0A;
FIELD(x19;=OperationCode)LEN(x02;x00;)VALUE(x36;x33;="63"=>user-command)
FIELD(x40;=CommandMode)LEN(x01;x00;)VALUE(x31;="0"=>send)
FIELD(x41;=CommandType)LEN(x02;x00;)VALUE(x31;x36;="17"=>data)
FIELD(x46;=)LEN(x93;x00;)VALUE(x50;x4F;x53;x54;x20;x2F;x73;x65;x72;x76;x69;x63;x65;x2D;x73;x65;x6C;x65;x63;x74;x6F;x72;x2F;x20;x48;x54;x54;x50;x2F;x31;x2E;x31;x0D;x0A;x48;x6F;x73;x74;x3A;x20;x39;x31;x2E;x31;x30;x37;x2E;x36;x35;x2E;x36;x36;x3A;x31;x39;x37;x39;x0D;x0A;x43;x6F;x6E;x74;x65;x6E;x74;x2D;x54;x79;x70;x65;x3A;x20;x61;x70;x70;x6C;x69;x63;x61;x74;x69;x6F;x6E;x2F;x6F;x63;x74;x65;x74;x2D;x73;x74;x72;x65;x61;x6D;x0D;x0A;x43;x6F;x6E;x74;x65;x6E;x74;x2D;x4C;x65;x6E;x67;x74;x68;x3A;x20;x33;x34;x31;x0D;x0A;x43;x61;x63;x68;x65;x2D;x43;x6F;x6E;x74;x72;x6F;x6C;x3A;x20;x6E;x6F;x2D;x73;x74;x6F;x72;x65;x0D;x0A;x0D;x0A;)

>x06;
ACK(x06;)

>x19;x02;x00;x36;x33;x41;x02;x00;x31;x37;x43;x01;x00;x30;
FIELD(x19;=OperationCode)LEN(x02;x00;)VALUE(x36;x33;="63"=>user-command)
FIELD(x41;=CommandType)LEN(x02;x00;)VALUE(x31;x36;="17"=>data)
FIELD(x43;=CommandResult)LEN(x01;x00;)VALUE(x30;="0"=>SUCCESS)

// recv data
x19;x02;x00;x36;x33;x40;x01;x00;x31;x41;x02;x00;x31;x37;
FIELD(x19;=OperationCode)LEN(x02;x00;)VALUE(x36;x33;="63"=>user-command)
FIELD(x40;=CommandMode)LEN(x01;x00;)VALUE(x31;="1"=>recv)
FIELD(x41;=CommandType)LEN(x02;x00;)VALUE(x31;x37;="17"=>data)

>x06;
ACK(x06;)

// conn close
x19;x02;x00;x36;x33;x40;x01;x00;x30;x41;x02;x00;x31;x36;x46;x12;x00;x39;x31;x2E;x31;x30;x37;x2E;x36;x35;x2E;x36;x36;x3B;x31;x39;x37;x39;x3B;
FIELD(x19;=OperationCode)LEN(x02;x00;)VALUE(x36;x33;="63"=>user-command)
FIELD(x40;=CommandMode)LEN(x01;x00;)VALUE(x31;="0"=>close)
FIELD(x41;=CommandType)LEN(x02;x00;)VALUE(x31;x36;="16"=>conn)
FIELD(x46;=)LEN(x12;x00;)VALUE(x39;x31;x2E;x31;x30;x37;x2E;x36;x35;x2E;x36;x36;x3B;x31;x39;x37;x39;x3B;=91.107.65.66;1979;)
*/

#define INPAS_MANUFACTURER	"INP"
#define INPAS_MODEL			"PAX D200"
#define INPAS_PACKET_SIZE 512
#define INPAS_COLLECT_SIZE 4000
#define INPAS_CRC_SIZE 2
#define INPAS_INIT_DELAY 2000
#define INPAS_INIT_TIEMOUT 60000
#define INPAS_RECV_TIMEOUT 1000
#define INPAS_WAIT_DELAY 250
#define INPAS_CANCEL_TIMEOUT 180000
#define INPAS_CANCEL_TRY_NUMBER 10
#define INPAS_QRCODE_TIMEOUT 20000
#define INPAS_VERIFICATION_TIEMOUT 120000

enum Control {
	Control_SOH	 = 0x01,
	Control_STX  = 0x02,
	Control_ETX	 = 0x03,
	Control_EOT	 = 0x04,
	Control_ACK	 = 0x06,
	Control_NAK	 = 0x15,
};

enum HeaderTlv {
	HeaderTlv_Version		 = 0x01,
	HeaderTlv_Number		 = 0x02,
};

enum Tlv {
	Tlv_Credit				 = 0x00,
	Tlv_Currency			 = 0x04,
	Tlv_CardNumber			 = 0x0A,
	Tlv_OperationCode		 = 0x19, //25
	Tlv_TerminalId			 = 0x1B, //27
	Tlv_TransactionResult	 = 0x27,
	Tlv_PrintTimeout		 = 0x39, //57
	Tlv_PrintText			 = 0x47, //71
	Tlv_CommandMode			 = 0x40, //64
	Tlv_CommandType			 = 0x41, //65
	Tlv_CommandResult		 = 0x43, //67
	Tlv_CommandData			 = 0x46, //70
	Tlv_CashierRequest		 = 0x4C, //76
	Tlv_MdbOptions			 = 0x56,
	Tlv_CheckShape			 = 0x5A, //90
};

enum Operation {
	Operation_Sale			 = 1,
	Operation_Wait			 = 21,
	Operation_CheckLink		 = 26,
	Operation_Message		 = 41,
	Operation_Cancel		 = 53,
	Operation_Verification	 = 59,
	Operation_Net			 = 63,
};

enum CommandType {
	CommandType_Conn = 16,
	CommandType_Data = 17,
};

enum CommandConnMode {
	CommandConn_Open	 = 1,
	CommandConn_Close	 = 0,
};

enum CommandDataMode {
	CommandData_Recv	 = 1,
	CommandData_Send	 = 0,
};

enum TransactionResult {
	TransactionResult_Unknown			 = 0,
	TransactionResult_Success			 = 1,
	TransactionResult_Denied			 = 0x01, //16
	TransactionResult_ConnectionError	 = 0x22, //34
	TransactionResult_Aborted			 = 0x35, //53
};

#pragma pack(push,1)
struct SohHeader {
	BEUint2 len;
	uint8_t data[0];
};

struct TlvHeader {
	uint8_t type;
	BEUint2 len;
	uint8_t data[0];
};
#pragma pack(pop)

class PacketLayerObserver {
public:
	enum Error {
		Error_ResponseTimeout	 = 0,
		Error_PacketFormat		 = 1,
	};

	virtual ~PacketLayerObserver() {}
	virtual void procPacket(const uint8_t *data, const uint16_t len) = 0;
	virtual void procControl(uint8_t control) = 0;
	virtual void procError(Error error) = 0;
};

class PacketLayerInterface {
public:
	virtual ~PacketLayerInterface() {}
	virtual void setObserver(PacketLayerObserver *observer) = 0;
	virtual void reset() = 0;
	virtual bool sendPacket(Buffer *data) = 0;
	virtual bool sendControl(uint8_t control) = 0;
};

class TlvPacket {
public:
	TlvPacket(uint16_t nodeMaxNumber);
	~TlvPacket();
	bool parse(const uint8_t *data, uint16_t dataLen);
	bool getNumber(uint8_t id, uint16_t *num);
	bool getNumber(uint8_t id, uint32_t *num);
	bool getNumber(uint8_t id, uint64_t *num);
	bool getString(uint8_t id, StringBuilder *str);
	bool getDateTime(uint8_t id, DateTime *date);
	bool getMdbOptions(uint8_t id, uint32_t *decimalPoint, uint32_t *scaleFactor, uint8_t *version);
	TlvHeader *find(uint8_t id);

private:
	TlvHeader **nodes;
	int nodeNum;
	int nodeMax;
};

class TlvPacketMaker {
public:
	TlvPacketMaker(uint16_t bufSize);
	Buffer *getBuf();
	const uint8_t *getData();
	uint16_t getDataLen();
	bool addNumber(uint8_t id, uint16_t dataSize, uint32_t num);
	bool addString(uint8_t id, const char *str, uint16_t strLen);
	void clear();

private:
	Buffer buf;
};

/*
 * Полином: X^16 + X^15 + X^2 + 1
 */
class Crc {
public:
	void start() { crc = 0; }

	void add(uint8_t byte) {
		for(int j = 0; j < 8; j++) {
			int x16 = (((byte & 0x80) && (crc & 0x8000)) || ((!(byte & 0x80)) && (!(crc & 0x8000)))) ? 0 : 1;
			int x15 = (((x16) && (crc & 0x4000)) || ((!x16) && (!(crc & 0x4000)))) ? 0 : 1;
			int x2 = (((x16) && (crc & 0x0002)) || ((!x16) && (!(crc & 0x0002)))) ? 0 : 1;
			crc = crc << 1;
			byte = byte << 1;
			crc |= (x16) ? 0x0001 : 0;
			crc = (x2) ? crc | 0x0004 : crc & 0xfffb;
			crc = (x15) ? crc | 0x8000 : crc & 0x7fff;
		}
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
	uint16_t crc;
};

}

#endif
