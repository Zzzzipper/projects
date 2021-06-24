#ifndef COMMON_INPAS_PROTOCOL_H
#define COMMON_INPAS_PROTOCOL_H

#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "tcpip/include/TcpIp.h"
#include "utils/include/Hex.h"

#include <stdint.h>

namespace CciCsi {
/*
---------------------
ќсобенности протокола
---------------------
—труктура блока команды и ответа:
	STX
	<TYPE> тип пакета
	<data[N]> посылаемые данные (N байт)
	ETX
	<CRCH><CRCL> контрольна€ сумма
	ETB

>>>>> Identification

19.06.28 19:18:13 <02<58<03<35<42<17
STX(x02)Type(x58=X)ETX(x03)CRC(x35x42)ETB(x17)

19.06.28 19:18:13 >06>02>58>31>36>30>32>30>37>30>34>03>35>44>17
ACK(x06)
STX(x02)Type(x58=X)DATA(x31x36x30x32x30x37x30x34)ETX(x03)CRC(x35x44)ETB(x17)
I=x31=1,CSI (Coffee Standard Interface) (level 1)
T2(x36)T1(x30)=60,MDB coin changer
V3(x32)V2(x30)V1(x37)=207,interface version
L2(x30)L1(x34)=04,

>>>>> Check status

19.06.28 19:18:13 <02<53<03<35<30<17
STX(x02)Type(x53=S)ETX(x03)CRC(x35x30)ETB(x17)

19.06.28 19:18:14 >06>02>53>31>90>30>00>03>43>31>17
ACK(x06)
STX(x02)Type(x53=S)DATA(x31x90x30x00)ETX(x03)CRC(x43x31)ETB(x17)
X=x31,1=>Ready
IF_STAT=x90=>CREDIT_HIDDEN
TO_PS=x30=>0,Timeout
RESERVED=x00

>>>>> Sale request => deny

19.06.28 19:51:24 <02<49<32<30<31<31<03<34<38<17
STX(x02)Type(x49=I)DATA(x32x30x31x31)ETX(x03)CRC(x34x38)ETB(x17)
N3(x32)N2(x30)N1(x31)=201=>productId
E(x31)=1,withdraw if okay

19.06.28 19:51:24 >06>02>49>30>03>37>41>17
ACK(x06)
STX(x02)Type(x49=I)DATA(x30)ETX(x03)CRC(x37x41)ETB(x17)
X=x30=>0=>credit low

>>>>> Sale request => approve

19.06.28 19:52:54 <02<49<32<30<31<31<03<34<38<17
STX(x02)Type(x49=I)DATA(x32x30x31x31)ETX(x03)CRC(x34x38)ETB(x17)
N3(x32)N2(x30)N1(x31)=201=>productId
E(x31)=1,withdraw if okay

19.06.28 19:52:54 >06>02>49>31>03>37>42>17
ACK(x06)
STX(x02)Type(x49=I)DATA(x31)ETX(x03)CRC(x37x42)ETB(x17)

*/

#define INPAS_MANUFACTURECCICSI_PACKET_MAX_SIZE_MODEL			"PAX-D 200"
#define CCICSI_PACKET_MAX_SIZE 512 // todo: уточнить
#define CCICSI_CRC_SIZE 2
#define CCICSI_RECV_TIMEOUT 1000
#define CCICSI_STATUS_MAX 10

enum Control {
	Control_STX  = 0x02,
	Control_ETX	 = 0x03,
	Control_ETB	 = 0x17,
	Control_ACK	 = 0x06,
	Control_NAK	 = 0x15,
};

// FRANKE T,S,X,M,I,C,B,R,D
enum Operation {
	Operation_Status		 = 0x53, // S/83
	Operation_Identification = 0x58, // X/88
	Operation_Inquiry		 = 0x49, // I/73
	Operation_BillingEnable	 = 0x56, // V/86
	Operation_Buttons		 = 0x54, // T/84
	Operation_MachineMode	 = 0x4D, // M/77
	Operation_Telemetry		 = 0x44, // D/68
	Operation_C				 = 0x43, // C/67
	Operation_B				 = 0x42, // B/66
	Operation_R				 = 0x52, // R/82
	// R
};

enum Interface {
	Interface_None = 0x30,
	Interface_CSI  = 0x31, // coffee machine
	Interface_CCI  = 0x32, // payment system
	Interface_GABI = 0x33, // GAstro Bedienungsinterface
};

enum T1 {
	T1_Coin = 0,
	T1_MdbCoinChanger = 60,
	T1_MdbBillValidator = 61,
	T1_MdbCardReader = 62
};

enum Status {
	Status_NoAction				= 0x30,
	Status_Ready				= 0x31,
	Status_Busy					= 0x32,
	Status_ErrorPaymentSystem	= 0x33,
	Status_ErrorInterface		= 0x34,
};

enum Verdict {
	Verdict_Deny = 0x30,
	Verdict_Approve = 0x31,
};

#pragma pack(push,1)
struct Header {
	uint8_t type;
	uint8_t data[0];
};

struct IdentificationResponse {
	uint8_t type;
	uint8_t interface;
	LEUnum2 paymentSystem;
	LEUnum3 version;
	LEUnum2 level;
};

struct StatusResponse  {
	uint8_t type;		// Type = 'S'
	uint8_t status;		// x = status: 0: no action 3: error payment system 1: ready  4: error interface 2: busy
	uint8_t IF_STAT;	// IF_STAT different States (see below)
	LEUnum1 TO_PS;		// TO_PS Timeout_Payment system: response time to an INQUIRY telegram
	uint8_t reserved;	// reserved 0xFF
};

struct InquiryRequest {
	uint8_t type;		// Type = 'I'
	LEUnum3 articleNumber;
	LEUnum1 exec;
};

struct InquiryResponse {
	uint8_t type;		// Type = 'I'
	uint8_t verdict;
};
/*
m_sendBuf[0] = CM_STX;
m_sendBuf[1] = 'T';
m_sendBuf[2] = '9';
m_sendBuf[3] = m_parser->intToChar((m_buttonsState >> 12) & 0xF);
m_sendBuf[4] = m_parser->intToChar((m_buttonsState >> 8) & 0xF);
m_sendBuf[5] = m_parser->intToChar((m_buttonsState >> 4) & 0xF);
m_sendBuf[6] = m_parser->intToChar(m_buttonsState & 0xF);
m_sendBuf[7] = '0';
m_sendBuf[8] = '0';
m_sendBuf[9] = '0';
if (m_operator->isPackageRequest())
	m_sendBuf[10] = m_parser->intToChar(m_operator->getPackageNumber());
else
	m_sendBuf[10] = '0';
*/
struct ButtonsResponse {
	uint8_t type;		// Type = 'T'
	uint8_t magic1;		// '9' wft?
	LEUhex4 state;		// button state
	uint8_t magic2[3];	// '0' wtf?
	uint8_t magic3;
};

struct MachineModeResponse {
	uint8_t type;		// Type = 'M'
	uint8_t magic1;		// 0x80 wft?
	uint8_t magic2;		// 0x80 wft?
};

struct TelemetryResponse {
	uint8_t type;		// Type = 'D'
	uint8_t magic1;		// '1' wft?
};

struct CResponse {
	uint8_t type;		// Type = 'C'
	uint8_t magic1;		// 0x30 wft?
	uint8_t magic2;		// 0x30 wft?
	uint8_t magic3;		// 0x30 wft?
	uint8_t magic4;		// 0x30 wft?
	uint8_t magic5;		// 0x30 wft?
	uint8_t magic6;		// 0x30 wft?
	uint8_t magic7;		// 0x30 wft?
};

/*
x42;type
x31;x30;x30;articleNumber
x30;x30;x30;x30;x30;x30;magic1
x30;exec
x30;x30;*/
struct AmountRequest {
	uint8_t type;		// Type = 'I'
	LEUnum3 articleNumber;
	uint8_t magic1[6];
	LEUnum1 exec;
	uint8_t magic2[2];
};

struct BResponse {
	uint8_t type;		// Type = 'B'
	uint8_t magic1;		// 0x30 wft?
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

class Crc {
public:
	void start() { crc = 0; }

	void add(uint8_t byte) {
		crc ^= byte;
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
		return digitToHex(0x0F & (crc >> 4));
	}

	uint8_t getLowByte() {
		return digitToHex(0x0F & crc);
	}

private:
	uint8_t crc;
};

}

#endif
