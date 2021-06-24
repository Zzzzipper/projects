#ifndef COMMON_VENDOTEK_PROTOCOL_H
#define COMMON_VENDOTEK_PROTOCOL_H

#include "cardreader/vendotek/Tlv.h"
#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "tcpip/include/TcpIp.h"

#include <stdint.h>

namespace Vendotek {
/*
---------------------
Особенности протокола
---------------------
Структура блока команды и ответа:
	0x1F
	<DESCRIMINATOR> - 0x96FB - from VMC; 0x97FB - from POS; (Big-Endian)
	<LENH><LENL> размер данных (только данных без контрольной суммы)(Big-Endian)
	<data[N]> посылаемые данные (N байт)
	<CRCH><CRCL> контрольная сумма (Big-Endian)

TLV-формат стандартный ISO/IEC 8825-1
Кодировка текста - win1251?

request
1F001196FB010349444C0308303030303030303010F8
1F - Starting byte
0011 - Length (big-endian)
96FB - Protocol discriminator (big-endian). 0x96FB - from VMC
01 - Message name tag
03 - Length of message name
49444C - message name IDL
03 - Operation number tag
08 - Length of operation number
3030303030303030 - number
10 F8 - CRC16

response
1F001297FB010349444C030130060331323008013067C6
1F - Starting byte
0012 - Length
97FB - Protocol discriminator (big-endian). 0x97FB - from
01/03/49444C => message name = IDL
03/01/30 => operation = 1
06/03/313230
08/01/30
67C6 - CRC16

Вопросы:
- почему не отобразился QR-код?
- как включить прием MIFARE?
- как выводить свой текст на экран?
- как принудительно убрать напдись "выберите товар"
- нужна кнопка отмены оплаты на экране
- нужна версия вендотека работающая через модем

1F 00 22 96 FB
01 03 49 44 4C MessageName=IDL
11 14 32 30 32 30 30 33 31 36 54 31 36 30 32 34 32 2B 30 33 30 30 LocalTime=20200316T160242+0300
0A 03 2E 2E 2E
79 A9
*/

#define VENDOTEK_MANUFACTURER	"VTK"
#define VENDOTEK_MODEL			"VENDOTEK3"
#define VENDOTER_HEADER_SIZE 128
#define VENDOTEK_DATA_SIZE 512
#define VENDOTEK_PACKET_SIZE (VENDOTER_HEADER_SIZE + VENDOTEK_DATA_SIZE)
#define VENDOTEK_CRC_SIZE 2
#define VENDOTEK_MESSAGENAME_SIZE 3
#define VENDOTEK_KEEPALIVE_TIMEOUT 1000
#define VENDOTEK_RECV_TIMEOUT 8000
#define VENDOTEK_RESPONSE_TIMER 10
#define VENDOTEK_REQUEST_TIMEOUT 20000
#define VENDOTEK_APPROVING_TIMEOUT 120000
#define VENDOTEK_VENDING_TIMEOUT 120000
#define VENDOTEK_QRCODE_TIMEOUT 20000
#define VENDOTEK_RESULT_TIMEOUT 4000

enum Control {
	Control_START = 0x1F,
};

enum Discriminator {
	Discriminator_VMC = 0x96FB,
	Discriminator_POS = 0x97FB,
};

enum Type {
	Type_MessageName			= 0x01,
	Type_OperationNumber		= 0x03,
	Type_Amount					= 0x04,
	Type_KeepaliveInterval		= 0x05,
	Type_OperationTimeout		= 0x06,
	Type_EventName				= 0x07,
	Type_EventNumber			= 0x08,
	Type_ProductId				= 0x09,
	Type_QrCode					= 0x0A,
	Type_TcpIpDestination		= 0x0B,
	Type_OutgoingByteCounter	= 0x0C,
	Type_SimpleDataBlock		= 0x0D,
	Type_ConfirmableDataBlock	= 0x0E,
	Type_ProductName			= 0x0F,
	Type_LocalTime				= 0x11,
	Type_SystemInformation		= 0x12,
	Type_BankingReciept			= 0x13,
	Type_DisplayTime			= 0x14,
};

enum ConnStatus {
	ConnStatus_NoService		= 0x00,
	ConnStatus_Established		= 0x01,
	ConnStatus_Attempts			= 0x02, // connection attempts were never made
	ConnStatus_LinkInactive		= 0x03,
	ConnStatus_Timeout			= 0x04,
	ConnStatus_ClosedByLocal	= 0x05, // closed by local host
	ConnStatus_ClosedByRemote	= 0x06, // closed by remote host
};

#pragma pack(push,1)
struct TcpIpDestination {
	uint8_t index;
	BEUint4 addr;
	LEUint2 port;
	uint8_t result;
	LEUint2 windowSize;
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

/*
 * CRC16-CCITT
 */
class Crc {
public:
	void start();
	void add(uint8_t byte);
	void add(uint8_t *data, uint16_t dataLen);
	uint16_t getCrc();
	uint8_t getHighByte();
	uint8_t getLowByte();

private:
	uint16_t crc;
};

}

#endif
