#ifndef COMMON_INGENICO_PROTOCOL_H
#define COMMON_INGENICO_PROTOCOL_H

#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "tcpip/include/TcpIp.h"

#include <stdint.h>

namespace Ingenico {
/*
---------------------
Особенности протокола
---------------------
Структура блока команды и ответа:
	SOH (0x01)
	LEN-HB
	LEN-LB

Вопрос:
1. каков максимальный размер пакета?
2. Как прервать текущую оплату?
3. Как вывести QR-кода на экран?

-------------
Пример оплаты
-------------
>>x31;x1B;x31;x1B;x36;x34;x33;x1B;x31;x30;x30;x2E;x30;x31;x1B;x1B;
x31;x1B; // class = 1 = fin
x31;x1B; // operation = 1 = pay
x36;x34;x33;x1B; // currency = 643
x31;x30;x30;x2E;x30;x31;x1B; // value = 100.001
x1B;

<<x53;x54;x41;x54;x55;x53;x3A;xD1;xE2;xE5;xF0;xEA;xE0;x20;xE8;xF2;xEE;xE3;xEE;xE2;
x53;x54;x41;x54;x55;x53;x3A; // STATUS:
xD1;xE2;xE5;xF0;xEA;xE0;x20;xE8;xF2;xEE;xE3;xEE;xE2; // Сверка итогов
>>OK

<<x53;x54;x41;x54;x55;x53;x3A;xCF;xEE;xE4;xEA;xEB;x2E;x20;x45;x74;x68;x65;x72;x6E;x65;x74;
STATUS:Подкл. Ethernet
>>OK

<<x44;x45;x56;x49;x43;x45;x4F;x50;x45;x4E;x3A;x53;x4F;x43;x4B;x45;x54;
DEVICEOPEN:SOCKET
>>OK:<socket-num>

<<x49;x4F;x43;x54;x4C;x3A;x30;x3A;x32;x30;x30;x30;x3A;x32;x30;x30;x30;
IOCTL:<socket-num>:2000:2000
>>OK

<<x43;x4F;x4E;x4E;x45;x43;x54;x3A;x30;x3A;x38;x30;x2E;x32;x34;x35;x2E;x31;x31;x37;x2E;x32;x32;x30;x3A;x31;x39;x31;x31;x31;
CONNECT:<socket-num>:80.245.117.220:19111
>>OK

<<x53;x54;x41;x54;x55;x53;x3A;xCF;xEE;xE4;xEA;xEB;xFE;xF7;xE5;xED;xE8;xE5;x20;x53;x53;x4C;
STATUS:Подключение SSL2
>>OK

<<x49;x4F;x43;x54;x4C;x3A;x30;x3A;x35;x3A;x35;
IOCTL:0:5:5
>>OK

<<x52;x45;x41;x44;x3A;x30;x3A;x31;x31;x30;x30;
READ:0:1100
>>OK

<<x44;x49;x53;x43;x4F;x4E;x4E;x45;x43;x54;x3A;x30;
DISCONNECT:0
>>OK

<<x44;x45;x56;x49;x43;x45;x43;x4C;x4F;x53;x45;x3A;x30;
DEVICECLOSE:0
>>OK
// ----
// Инфо
// ----
x32;x1B; // class = 2
x32;x31;x1B; // opearation = 21
x1B;x1B;x1B;

x01;x00;x06;
x50;x49;x4E;x47;x3A; // PING:
x30; // 0
*/

#define INGENICO_MANUFACTURER	"ING"
#define INGENICO_MODEL			"IPP-320"
#define INGENICO_PACKET_SIZE 1200 //todo: разобраться почему с буфером меньше 1000 не работает
#define INGENICO_NETWORK_SIZE INGENICO_PACKET_SIZE - 3
#define INGENICO_RECV_TIMEOUT 5000

enum Control {
	Control_SOH = 0x01,
	Control_ESC = 0x1B,
};

enum Tlv {
	Tlv_Credit				 = 0x00,
	Tlv_Currency			 = 0x04,
	Tlv_OperationCode		 = 0x19, //27
	Tlv_CardNumber			 = 0x0A,
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
	Operation_Sale	 = 1,
	Operation_Wait	 = 21,
	Operation_Cancel = 53,
	Operation_Net	 = 63,
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
	TransactionResult_Success = 0x01,
};

#pragma pack(push,1)
struct TlvHeader {
	uint8_t type;
	BEUint2 len;
	uint8_t data[0];
};
#pragma pack(pop)

class Packet {
public:
	Packet(uint16_t size);
	void clear();
	void addControl(uint8_t control);
	void addSymbol(char symbol);
	void addString(const char *str);
	void addString(const char *str, uint16_t strLen);
	void addNumber(uint32_t number);
	Buffer *getBuf();
	uint8_t *getData();
	uint32_t getDataLen();

private:
	Buffer buf;
};

class PacketLayerObserver {
public:
	enum Error {
		Error_ResponseTimeout	 = 0,
		Error_PacketFormat		 = 1,
	};

	virtual ~PacketLayerObserver() {}
	virtual void procPacket(const uint8_t *data, const uint16_t len) = 0;
	virtual void procError(Error error) = 0;
};

class PacketLayerInterface {
public:
	virtual ~PacketLayerInterface() {}
	virtual void setObserver(PacketLayerObserver *observer) = 0;
	virtual void reset() = 0;
	virtual bool sendPacket(Buffer *data) = 0;
};

}

#endif
