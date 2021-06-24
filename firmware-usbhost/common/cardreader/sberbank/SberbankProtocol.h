#ifndef COMMON_SBERBANK_PROTOCOL_H
#define COMMON_SBERBANK_PROTOCOL_H

#include "utils/include/NetworkProtocol.h"
#include "utils/include/Buffer.h"

#include <stdint.h>

/*
// lan/open
CMD(xA0;=CMD_MASTERCALL)
LEN(x11;x00;)
NUM(x04;x00;x00;x00;)
ISTRUCTION(x01;=OPEN)
DEV(x19;=LAN)
RESERVED(x00;)
PARAMLEN(x0C;x00;)
RESERVED(x00;x00;)
IPADDR(xC2;x36;x0E;x18;=194.54.14.24)
PORT(x9C;x02;=668)
TIMEOUT(x20;x4E;x00;x00;=20000)

// lan/read
CMD(xA0;=CMD_MASTERCALL)
LEN(x0B;x00;)
NUM(x17;x00;x00;x00;)
ISTRUCTION(x02;=READ)
DEV(x19;=LAN)
RESERVED(x00;)
PARAMLEN(x06;x00;)
READLEN(x00;x04;=1024)
TIMEOUT(x2C;x01;x00;x00;=300)?

// lan/read/response
CMD(x00;)
LEN(x11;x00;)
NUM(x19;x00;x00;x80;)
ISTRUCTION(x02;=READ)
DEV(x19;=LAN)
RESERVED(x00;)
PARAMLEN(x01;x00;)
REDVDATA(x05;)

// lan/write
CMD(xA0;=CMD_MASTERCALL)
LEN(x51;x00;)
NUM(x0F;x00;x00;x00;)
ISTRUCTION(x03;=WRITE)
DEV(x19;=LAN)
RESERVED(x00;)
PARAMLEN(x4C;x00;)
WRITEDATA(...)

// lan/write/response
CMD(x00;)
LEN(x11;x00;)
NUM(x13;x00;x00;x80;)
ISTRUCTION(x03;=WRITE)
DEV(x19;=LAN)
RESERVED(x00;)
PARAMLEN(x02;x00;)
SENDLEN(x4C;x00;)

// lan/close
CMD(xA0;=CMD_MASTERCALL)
LEN(x05;x00;)
NUM(x18;x00;x00;x00;)
ISTRUCTION(x04;=CLOSE)
DEV(x19;=LAN)
RESERVED(x00;)
PARAMLEN(x00;x00;)

// ???/write
CMD(xA0;=CMD_MASTERCALL)
LEN(x08;x00;)
NUM(x1A;x00;x00;x00;)
ISTRUCTION(x03;=WRITE)
DEV(x2B;=???)
RESERVED(x00;)
PARAMLEN(x03;x00;)
PARAMDATA(x00;x00;x00;)

// printer/open
CMD(xA0;=CMD_MASTERCALL)
LEN(x08;x00;)
NUM(x2A;x00;x00;x00;)
ISTRUCTION(x01;=OPEN)
DEV(x03;=???)
RESERVED(x00;)
PARAMLEN(x03;x00;)
PARAMDATA(x00;x00;x00;)

// qrcode/req
CMD(x6d;=CMD_MASTERCALL)
LEN(x8d;x00;)
NUM(x8b;x72;x08;x00;)
SUM(00 00 00 00)
CARDTYPE(00)
CURRENCY(00)
OPERATION(39)
CARDROAD2(
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00)
OPERID(65 fa 43 96)
RRN(00 00 00 00 00 00 00 00 00 00 00 00 00)
FLAGS(00 04 00 00)
PARAMLEN(48)
PARAMVAL(df 55 45 74 3d 32 30 31 37 30 32 31 36 54 31 33 33 36 30 30 26 73 3d 37 38 30 2e 30 30 26 66 6e 3d 39 39 39 39 30 37 38 39 30 30 30 30 31 33 32 37 26 69 3d 39 31 26 66 70 3d 31 36 39 37 30 31 33 35 35 37 26 6e 3d 31)
PARAMVAL(DF;55;45;74;3D;32;30;31;37;30;32;31;36;54;31;33;33;36;30;30;26;73;3D;37;38;30;2E;30;30;26;66;6E;3D;39;39;39;39;30;37;38;39;30;30;30;30;31;33;32;37;26;69;3D;39;31;26;66;70;3D;31;36;39;37;30;31;33;35;35;37;26;6E;3D;31;)

// qrcode/req2
CMD(x6d;=CMD_MASTERCALL)
LEN(x8d;x00;)
NUM(xF7;x6E;x07;x00;)
SUM(00 00 00 00)
CARDTYPE(00)
CURRENCY(00)
OPERATION(39)
CARDROAD2(
x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;
x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;
x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;
x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;)
OPERID(x00;x00;x00;x00;)
RRN(x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;)
FLAGS(x00;x00;x00;x00;)
PARAMLEN(x48;)
PARAMVAL(xDF;x55;x45;x74;x3D;x32;x30;x31;x37;x30;x32;x31;x36;x54;x31;x33;x33;x36;x30;x30;x26;x73;x3D;x37;x38;x30;x2E;x30;x30;x26;x66;x6E;x3D;x39;x39;x39;x39;x30;x37;x38;x39;x30;x30;x30;x30;x31;x33;x32;x37;x26;x69;x3D;x39;x31;x26;x66;x70;x3D;x31;x36;x39;x37;x30;x31;x33;x35;x35;x37;x26;x6E;x3D;x31;)

// qrcode/resp todo: check qrcode response
x00;xBC;x00;xF7;x6E;x07;x80;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x01;x00;x00;x00;x0C;xEA;x5F;xB3;x60;xEA;x5F;xB3;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x5B;x16;x34;x01;x26;x22;x03;x00;x00;x30;x30;x31;x31;x39;x39;x38;x37;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;xD9;x97;xA6;x00;x00;x00;x00;x31;x31;x34;x34;x34;x34;x34;x34;x35;x35;x35;x35;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;
CMD(x00;=)
LEN(xBC;x00;)
NUM(xF7;x6E;x07;x80;)
x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x01;x00;x00;x00;x0C;xEA;x5F;xB3;x60;xEA;x5F;xB3;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x5B;x16;x34;x01;x26;x22;x03;x00;x00;x30;x30;x31;x31;x39;x39;x38;x37;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;xD9;x97;xA6;x00;x00;x00;x00;x31;x31;x34;x34;x34;x34;x34;x34;x35;x35;x35;x35;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;

// sale response
CMD(x00;=)
LEN(xBC;x00;)
NUM(xF6;x6E;x07;x80;)
result2(0000) // результат операции
authCode(32373033333500=270335) // кода авторизации
rrn(38333239353131353735343600=832951157546)// ссылочный номер RRN
operationNum(3030303100=0001) // номер операции за текущий день
cardNum(3432373636332a2a2a2a2a2a3239313500000000=427663******2915) // номер карты
cardValidity(30322f323000=02/20) // срок действия карты
textMessage(8e848e819085488e3a0000000000000000000000000000000000000000000000=ОДОБ? ЕHО:)
operationDate(85f03301=2018-11-25)
operationTime(460a0200=13:37:02)
ownerFlag(01)
terminalNum(323030323331343100=20023141) // номер терминала
cardName(5669736100000000000000000000000036333130303030303030343600000000=Visa  631000000046)
cardSha1(8171e79003bfc422a73796f0ee4e0c5ba05d4d56) // хеш от номера карты
cardSecret(d2634db744cab644740e69c9c4ddaa6b5f82c54f4a2097b251e70627a1357bc8) // зашифрованные данные
cardId(03)

// sr2
00BC00F66E0780E210584A324B5236003631393530343030303438340030303031002A2A2A2A2A2A2A2A2A2A2A2A3537393400D7E8BE31302F3232000000000000000000000000000000000000000000000000000000000000000000371734016B050200003131323539323036004D61737465726361726400BE9835060034303030303030313438393800000000BF621CE0DFC64D28C50940BF9B3617540BF20ADE6E129252D31AD7C25F5DC2A0A082BC5E36BF963878C45B098F7D6391B1D3ABDF01
CMD(00)
LEN(BC00)
NUM(F66E0780)
result2(E210)
authCode(584A324B523600)
rrn(36313935303430303034383400=619504000484)
operationNum(3030303100=0001)
cardNum(2A2A2A2A2A2A2A2A2A2A2A2A3537393400D7E8BE=************5794)
cardValidity(31302F323200=10/22)
textMessage(0000000000000000000000000000000000000000000000000000000000000000)
operationDate(37173401)
operationTime(6B050200)
ownerFlag(00)
terminalNum(313132353932303600)
cardName(4D61737465726361726400BE9835060034303030303030313438393800000000)
cardSha1(BF621CE0DFC64D28C50940BF9B3617540BF20ADE)
cardSecret(6E129252D31AD7C25F5DC2A0A082BC5E36BF963878C45B098F7D6391B1D3ABDF01)

26:25>00BC00F66E0780E210314553354F35003631393530343031373935370030303033002A2A2A2A2A2A2A2A2A2A2A2A3537393400D7E8BE31302F323200000000000000000000000000000000000000000000000000000000000000000037173401FE050200003131323539323036004D61737465726361726400BE9835060034303030303030313438393800000000BF621CE0DFC64D28C50940BF9B3617540BF20ADE6E129252D31AD7C25F5DC2A0A082BC5E36BF963878C45B098F7D6391B1D3ABDF01



x00;
xBC;x00;
xF6;x6E;x07;x80;
result2(xE2;x10;
authCode(x34;x39;x4C;x38;x30;x35;x00;)
rrn(x31;x35;x36;x39;x38;x33;x30;x33;x36;x35;x35;x39;x00;)
operationNum(x30;x30;x30;x31;x00;)
cardNum(x2A;x2A;x2A;x2A;x2A;x2A;x2A;x2A;x2A;x2A;x2A;x2A;x36;x36;x33;x39;x00;x05;x00;x00;
cardValidity(x30;x33;x2F;x32;x32;x00;
textMessage(x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;
operationDate(xD2;x16;x34;x01;
operationTime(xC2;x9D;x01;x00;
ownerFlag(x01;
terminalNum(x30;x30;x31;x31;x39;x39;x38;x37;x00;
cardName(x56;x69;x73;x61;x00;xEA;x5F;xB3;x5C;x2A;x10;x00;x00;x00;x00;x00;x31;x31;x34;x34;x34;x34;x34;x34;x35;x35;x35;x35;x00;x00;x00;x00;
cardSha1(x9B;xB3;xD7;x3D;x07;xFD;x76;x73;xB0;x1C;xCE;x66;x25;xD4;x7F;x8D;x18;x3C;xD5;x57;
cardSecret(xD2;x63;x4D;xB7;x44;xCA;xB6;x44;x19;x23;x93;xE3;x61;x7A;x3C;x67;x8B;x38;x33;x9B;xA0;xC0;x68;xEF;xDE;x71;x17;xD4;x56;xE2;xC1;x95;
cardId(x03;

struct PaymentResponse {
	uint8_t result;
	BEUint2 len;
	BEUint4 num;
	BEUint2 result2;
	uint8_t authCode[7];
	uint8_t rrn[13];
	BEUint5 operationNum;
	uint8_t cardNum[20];
	uint8_t cardValidity[6];
	uint8_t textMessage[32];
	uint8_t operationDate[4];
	uint8_t operationTime[4];
	uint8_t ownerFlag;
	uint8_t terminalNum[9];
	uint8_t cardName[32];
	uint8_t cardSha1[20];
	uint8_t cardSecret[32];
	uint8_t cardId;
};

 */

namespace Sberbank {

#define SBERBANK_MANUFACTURER	"SBR"
#define SBERBANK_MODEL			"PAX D200"
#define SBERBANK_COMMAND_SIZE 512
#define SBERBANK_PACKET_SIZE (SBERBANK_COMMAND_SIZE + 4)
#define SBERBANK_PACKET_TIMEOUT 1500
#define SBERBANK_FRAME_BODY_SIZE 180
#define SBERBANK_FRAME_SIZE ((SBERBANK_FRAME_BODY_SIZE + 4) * 4 / 3 + 10) // base64/data = 4/3
#define SBERBANK_FRAME_TIMEOUT 2500
#define SBERBANK_RESPONSE_FLAG 0x80000000
#define SBERBANK_RESPONSE_TIMER 10
#define SBERBANK_POLL_DELAY 500
#define SBERBANK_POLL_RECV_TIMEOUT 4000
#define SBERBANK_SVERKA_FIRST 5*60 // количество POLL-ов до сверки
#define SBERBANK_SVERKA_NEXT 8*60*60 // количество POLL-ов до сверки
#define SBERBANK_SVERKA_FIRST_TIMEOUT 30*1000 //5*60*1000
#define SBERBANK_SVERKA_NEXT_TIMEOUT  8*60*60*1000
#define SBERBANK_VERIFICATION_TIMEOUT 60000
#define SBERBANK_QRCODE_TIMEOUT 30000

enum Control {
	Control_STX  = 0x02,
	Control_ETX	 = 0x03,
	Control_EOT	 = 0x04,
	Control_ACK	 = 0x06,
	Control_BEL	 = 0x07,
	Control_NAK	 = 0x15,
	Control_MARK = 0x23,
};

enum Command {
	Command_Poll			 = 0xC0,
	Command_Transaction		 = 0x6D,
	Command_MasterCall		 = 0xA0,
	Command_Abort			 = 0x5B,
};

enum Operation {
	Operation_Payment		 = 0x01,
	Operation_PaymentCancel	 = 0x0D,
	Operation_SverkaItogov	 = 0x07,
	Operation_WaitButton	 = 0x3A,
	Operation_QrCode		 = 0x39,
};

enum Result {
	Result_OK = 0x00,

};

#pragma pack(push,1)
struct Frame {
	uint8_t num;
	uint8_t len;
	uint8_t data[0];
};

struct Packet {
	uint8_t command;
	BEUint2 len;
	BEUint4 num;
};

struct PollRequest {
	uint8_t command;
	BEUint2 len;
	BEUint4 num;
	BEUint4 param1;
	BEUint4 param2;
	BEUint4 param3;
};

struct PollResponse {
	uint8_t command;
	BEUint2 len;
	BEUint4 num;
	BEUint4 value;
};

struct TransactionRequest {
	uint8_t command;
	BEUint2 len;
	BEUint4 num;
	BEUint4 sum;
	uint8_t cardType;
	uint8_t depNum;
	uint8_t operation;
	uint8_t cardRoad2[40];
	BEUint4 requestId;
	uint8_t transactionNum[13];
	BEUint4 flags;
	uint8_t data[0];
};

struct PaymentRequest {
	uint8_t command;
	BEUint2 len;
	BEUint4 num;
	BEUint4 sum;
	uint8_t cardType;
	uint8_t depNum;
	uint8_t operation;
	uint8_t cardRoad2[40];
	BEUint4 requestId;
	uint8_t transactionNum[13];
	BEUint4 flags;
	uint8_t dataLen;
	uint8_t data[0];
};

struct BerTlv {
	LEUint2 tag;
	uint8_t len;
	uint8_t data[0];
};

struct PaymentResponse {
	uint8_t result;
	BEUint2 len;
	BEUint4 num;
	BEUint2 result2;
	uint8_t authCode[7];
	uint8_t rrn[13];
	BEUint5 operationNum;
	uint8_t cardNum[20];
	uint8_t cardValidity[6];
	uint8_t textMessage[32];
	uint8_t operationDate[4];
	uint8_t operationTime[4];
	uint8_t ownerFlag;
	uint8_t terminalNum[9];
	uint8_t cardName[28];
	BEUint4 points;
	uint8_t cardSha1[20];
	uint8_t cardSecret[32];
	uint8_t cardId;
};

enum Instruction {
	Instruction_Open = 0x01,
	Instruction_Read = 0x02,
	Instruction_Write = 0x03,
	Instruction_Close = 0x04,
};

enum Device {
	Device_None = 0x00,
	Device_Printer = 0x03,
	Device_Lan = 0x19,
	Device_Beeper = 0x20,
	Device_Reboot = 0x29,
};

struct MasterCallRequest {
	uint8_t result;
	BEUint2 len;
	BEUint4 num;
	uint8_t instruction;
	uint8_t device;
	uint8_t reserved;
	BEUint2 paramLen;
	uint8_t paramVal[0];
};

struct DeviceLanOpenParam {
	uint8_t reserved[2];
	uint8_t ipaddr[4];
	BEUint2 port;
	BEUint4 timeout;
};

struct DeviceLanReadParam {
	BEUint2 maxSize;
	uint8_t unknown[4];
};

struct DeviceLanSendResponse {
	BEUint2 sendLen;
};

struct DevicePrinterOpenParam {
	uint8_t mode;
};

struct DevicePrinterSendParam {
	uint8_t mode;
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
	virtual void procError(Error error) = 0;
};

class PacketLayerInterface {
public:
	virtual ~PacketLayerInterface() {}
	virtual void setObserver(PacketLayerObserver *observer) = 0;
	virtual void reset() = 0;
	virtual bool sendPacket(Buffer *data) = 0;
};

class FrameLayerObserver {
public:
	enum Error {
		Error_ResponseTimeout	 = 0,
		Error_PacketFormat		 = 1,
	};

	virtual ~FrameLayerObserver() {}
	virtual void procPacket(const uint8_t *data, const uint16_t len) = 0;
	virtual void procControl(uint8_t control) = 0;
	virtual void procError(Error error) = 0;
};

class FrameLayerInterface {
public:
	virtual ~FrameLayerInterface() {}
	virtual void setObserver(FrameLayerObserver *observer) = 0;
	virtual void reset() = 0;
	virtual bool sendPacket(Buffer *data) = 0;
	virtual bool sendControl(uint8_t control) = 0;
};

}

#endif
