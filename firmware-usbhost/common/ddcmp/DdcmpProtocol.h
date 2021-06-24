#ifndef COMMON_DDCMP_PROTOCOL_H
#define COMMON_DDCMP_PROTOCOL_H

#include "utils/include/NetworkProtocol.h"

/*
>05 06 40 00 08 01 5B 95
START:
COMMANDFLAG(05)
06 - type(START)
40 - quick sync flag
00 - always 0 in START message
08 - mdb(115200)
01 - station address (1)
CRC(5B95)

<05 07 40 02 08 01 C7 95
STACK:
COMMANDFLAG(05)
07 - type(STACK)
40 - quick sync flag
02 - wft?
08 - mdb(115200)
01 - station address (1)
CRC(C795)

>>000276:
>81 10 00 00 01 01 1F 82 77 E0 00 00 00 00 00 01
 03 19 20 19 17 00 00 0C 5D C9
DATAFLAG(81)
10 00 00 01 01 wtf?
CRC(1F 82)
(77 E0) - who are you
 00 - hz
(00 00) - security code
(00 00) - pass code
(01 03 19 20 19 17) - timemde
(00 00) - iruserid
(0C) - access type
CRC(5DC9)
where timemde
(01) - day
(03) - month
(19) - year%100
(20) - hour
(19) - minutes
(17) - seconds

<05 01 40 01 00 01 B8 55 81 15 40 01 01 01 97 82
 88 E0 01 00 00 00 00 F5 FD 06 05 04 03 02 01 00
 FF 00 00 00 A0 D2 D7
COMMANDFLAG(05)
TYPE(01)
 40 - quick sync flag
 01
 00
 01
CRC(B8 55)
DATAFLAG(81)
 15 40 01 01 01
CRC(97 82)
DATAFLAG(88)
 E0 01 00 00 00 00 F5 FD 06 05 04 03 02 01 00 FF 00 00 00 A0
CRC(D2D7)
 */
namespace Ddcmp {

#define DDCMP_STACK_TIMEOUT 250
#define DDCMP_TRY_NUMBER 5
#define DDCMP_RECV_TIMEOUT 5000
#define DDCMP_ACK_TIMEOUT 170
#define DDCMP_DELAY 65
#define DDCMP_PACKET_SIZE 255 //todo: уточнить

enum Message {
	Message_Control = 0x05,
	Message_Data = 0x81,
	Message_Request = 0x77,
	Message_Response = 0x88,
	Message_DataBlock = 0x99,
};

enum Type {
	Type_Start = 0x06,
	Type_Stack = 0x07,
	Type_Ack = 0x01,
	Type_Nack = 0x02,
	Type_WhoAreYou = 0xE0,
	Type_Read = 0xE2,
};

enum BaudRate {
	BaudRate_Unchanged = 0,
	BaudRate_1200 = 1,
	BaudRate_2400 = 2,
	BaudRate_4800 = 3,
	BaudRate_9600 = 4,
	BaudRate_19200 = 5,
	BaudRate_38400 = 6,
	BaudRate_57600 = 7,
	BaudRate_115200 = 8,
};

enum Flag {
	Flag_Maintenance = 0x0B,
	Flag_RoutePerson = 0x0C,
};

enum ListNumber {
	ListNumber_AuditOnly = 0x01,
	ListNumber_SecurityRead = 0x02,
	ListNumber_ConfigAndAudit = 50,
	ListNumber_MachineConfig = 64,
};

enum Constant {
	Constant_RecordNumber = 0x01,
	Constant_AuditData = 0x99,
	Constant_Finish = 0xFF,
};

enum DataFlag {
	DataFlag_Sync = 0x40,
	DataFlag_LastBlock = 0x80,
	DataFlag_LengthMask = 0x3F,
};

#pragma pack(push,1)
struct Header {
	uint8_t message;
	uint8_t type;
	uint8_t flags;
	uint8_t reserved[3];
	uint8_t crcLow;
	uint8_t crcHigh;
};

struct Control {
	uint8_t message;
	uint8_t type;
	uint8_t flags;
	uint8_t sbd;
	uint8_t mdr;
	uint8_t sadd;
};

struct HeaderResponse {
	uint8_t message;
	uint8_t type;
	uint8_t subtype;
};

struct WhoAreYouRequest {
	uint8_t message;
	uint8_t type;
	uint8_t zero1;
	LEUint2 securityCode;
	LEUint2 passCode;
	Ubcd1   dateDay;
	Ubcd1   dateMonth;
	Ubcd1   dateYear;
	Ubcd1   timeHour;
	Ubcd1   timeMinute;
	Ubcd1   timeSecond;
	LEUint2 userId;
	uint8_t flag;
};

struct WhoAreYouResponse {
	uint8_t message;
	uint8_t type;
	uint8_t subtype;
	LEUint2 securityCode;
	LEUint2 passCode;
	uint8_t machineSerialNumber[8];
	uint8_t softwareVersion;
	uint8_t manufacturer;
	uint8_t extraRead;
	uint8_t zero2;
	uint8_t msdb;
	uint8_t lsdb;
};

// 3.6.4.1.3 Read Data
struct StartRequest {
	uint8_t message;
	uint8_t type;
	uint8_t subtype;
	uint8_t listNumber;
	uint8_t recordNumber;
	BEUint2 byteOffset;
	BEUint2 segmentLength;
};

struct StartResponse {
	uint8_t message;
	uint8_t type;
	uint8_t subtype;
	uint8_t listNumber;
	uint8_t recordNumber;
	BEUint2 byteOffset;
	BEUint2 segmentLength;
};

struct ReadControl {
	uint8_t message;
	uint8_t len;
	uint8_t flags;
	uint8_t rx;
	uint8_t tx;
	uint8_t sadd;
};

struct ReadResponse {
	uint8_t wtf;
	uint8_t num;
	uint8_t data[0];
};

struct FinishRequest {
	uint8_t message;
	uint8_t type;
};

#pragma pack(pop)
/*
COMMANDFLAG(05)
06 - type(START)
40 - quick sync flag
00 - always 0 in START message
08 - mdb(115200)
01 - station address (1)
CRC(5B95)
 */

class PacketLayerObserver {
public:
	virtual ~PacketLayerObserver() {}
	virtual void recvControl(const uint8_t *data, const uint16_t len) = 0;
	virtual void recvData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen) = 0;
};

class PacketLayerInterface {
public:
	virtual ~PacketLayerInterface() {}
	virtual void setObserver(PacketLayerObserver *observer) = 0;
	virtual void reset() = 0;
	virtual void sendControl(uint8_t *cmd, uint16_t cmdLen) = 0;
	virtual void sendData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen) = 0;
};

class Crc {
public:
	void start() { crc = 0; }

	void add(uint8_t byte) {
		crc ^= byte; //CRC mit Polynom 0xa001 (1 + x**2 + x**15 + x**16)
		uint8_t bitcnt = 8;
		do {
			uint8_t flg = crc & 1;
			crc >>= 1;
			if(flg) {
				crc ^= 0xa001;
			}
		} while(--bitcnt);
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
