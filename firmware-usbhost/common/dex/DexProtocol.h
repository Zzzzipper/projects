/*
 * Protocol.h
 *
 * Created: 21.11.2015 17:13:28
 *  Author: Vladimir
 */

#ifndef COMMON_DEX_PROTOCOL_H_
#define COMMON_DEX_PROTOCOL_H_

#include <string.h>
#include <stdint.h>

#define DEX_DATA_MAX_SIZE 245
#define DEX_PACKET_MAX_SIZE 255
#define DEX_TIMEOUT_SENDING 20
#define DEX_TIMEOUT_ENQ_WAIT 2000
#define DEX_TIMEOUT_READING 4000 // CoinCo иногда сильно задумывается при передаче данных
#define DEX_TIMEOUT_INTERSESSION 100
#define DEX_TIMEOUT_RESTART 1000
#define DEX_COMMUNICATION_ID "EPHOR00001"
#define DEX_REVISION "R00"
#define DEX_LEVEL "L06"
#define DEX_REVISION2 "R01"
#define DEX_LEVEL2 "L01"
#define DEX_TRY_MAX 10

enum DexControl {
	DexControl_NUL = 0x00,
	DexControl_SOH = 0x01,
	DexControl_STX = 0x02,
	DexControl_ETX = 0x03,
	DexControl_EOT = 0x04,
	DexControl_ENQ = 0x05,
	DexControl_ACK = 0x06,
	DexControl_DLE = 0x10,
	DexControl_NAK = 0x15,
	DexControl_SYN = 0x16,
	DexControl_ETB = 0x17
};

enum DexOperation {
	DexOperation_Recv = 'R',
	DexOperation_Send = 'S',
	DexOperation_Manufacturer = 'M'
};

enum DexCommand {
	DexCommand_SlaveSend = 0,
	DexCommand_SlaveRecv,
	DexCommand_SlaveConfig,
	DexCommand_SlaveManufacturer,
	DexCommand_MasterSend,
	DexCommand_MasterRecv,
	DexCommand_MasterConfig,
	DexCommand_MasterManufacturer,
	DexCommand_Unsupported
};

#define DexResponse_OK "00"
#define DexResponse_UnrecognizedCommunication "01"
#define DexResponse_UnsupportedRevision "02"
#define DexResponse_OperationConflict "03"
#define DexResponse_NoData	"04"
#define DexResponse_UndefinedError "05"
#define DexResponse_ManufacturerState "90"

#pragma pack(push,1)
struct DexHandShakeRequest {
	uint8_t communicationId[10];
	uint8_t operation;
	uint8_t revision[3];
	uint8_t level[3];

	void set(const uint8_t operation) {
		memcpy(this->communicationId, DEX_COMMUNICATION_ID, sizeof(this->communicationId));
		this->operation = operation;
		memcpy(this->revision, DEX_REVISION2, sizeof(this->revision));
		memcpy(this->level, DEX_LEVEL2, sizeof(this->level));
	}
};
#pragma pack(pop)

/**
 * CoinCo Global2 зачем-то возвращает еще два дополнительных символа 0D и 0A.
 * Это нарушение протокола, но работать с ними надо всё равно (
 */
#pragma pack(push,1)
struct DexHandShakeResponse {
	uint8_t responseCode[2];
	uint8_t communicationId[10];
	uint8_t revision[3];
	uint8_t level[3];

	void set(const char *responseCode) {
		memcpy(this->responseCode, responseCode, sizeof(this->responseCode));
		memcpy(this->communicationId, DEX_COMMUNICATION_ID, sizeof(this->communicationId));
		memcpy(this->revision, DEX_REVISION2, sizeof(this->revision));
		memcpy(this->level, DEX_LEVEL2, sizeof(this->level));
	}

	bool isResponseCode(const char *responseCode) {
		return (sizeof(this->responseCode) == strlen(responseCode)) && (this->responseCode[0] == responseCode[0]) && (this->responseCode[1] == responseCode[1]);
	}
};
#pragma pack(pop)

#endif /* PROTOCOL_H_ */
