#ifndef COMMON_TWOCAN_PROTOCOL_H
#define COMMON_TWOCAN_PROTOCOL_H

#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "tcpip/include/TcpIp.h"

#include <stdint.h>

namespace Twocan {
/*
---------------------
ќсобенности протокола
---------------------
—труктура блока команды и ответа:
	STX
	<LENL><LENH> размер данных (только данных без контрольной суммы)
	<data[N]> посылаемые данные (N байт)
	<CRCH><CRCL> контрольна€ сумма

TLV-формат нестандартный (поле LEN всегда 2 байта)!
 одировка текста - win1251

¬опросы:
 */

#define TWOCAN_MANUFACTURER	"2CN"
#define TWOCAN_MODEL			"V20"
#define TWOCAN_PACKET_SIZE 512 // todo: уточнить
#define TWOCAN_CRC_SIZE 1
#define TWOCAN_INIT_DELAY 100
#define TWOCAN_INIT_TIEMOUT 60000
#define TWOCAN_RECV_TIMEOUT 1000
#define TWOCAN_WAIT_DELAY 25000    // +2 нул€
#define TWOCAN_CANCEL_TIMEOUT 180000
#define TWOCAN_CANCEL_TRY_NUMBER 10
#define TWOCAN_QRCODE_TIMEOUT 20000
#define TWOCAN_VERIFICATION_TIEMOUT 120000

enum Control {
	Control_STX  = 0x02,
	Control_ETX	 = 0x03,
	Control_EOT	 = 0x04,
	Control_ACK	 = 0x06,
	Control_NAK	 = 0x15,
	Control_PONG = 0x13,
	Control_PING = 0x14,
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
 * XOR всех байт
 */
class Crc {
public:
	void start(uint8_t byte) { crc = byte; }

	void add(uint8_t byte) {
		crc ^= byte;
	}

	void add(uint8_t *data, uint16_t dataLen) {
		for(uint16_t i = 0; i < dataLen; i++) {
			add(data[i]);
		}
	}

	uint8_t getCrc() {
		return crc;
	}

private:
	uint8_t crc;
};

}

#endif
