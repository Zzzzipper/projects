#ifndef COMMON_MDB_PROTOCOL_CASHLESS_H_
#define COMMON_MDB_PROTOCOL_CASHLESS_H_

#include "utils/include/NetworkProtocol.h"
#include "mdb/MdbProtocol.h"

#include <stdint.h>

namespace Mdb {
namespace Cashless {

enum Command {
	Command_Reset		= 0x00,
	Command_Setup		= 0x01,
	Command_Poll		= 0x02,
	Command_Vend		= 0x03,
	Command_Reader		= 0x04,
	Command_Revalue		= 0x05,
	Command_Expansion	= 0x07,
};

enum Subcommand {
	Subcommand_SetupConfig			 = 0x00,
	Subcommand_SetupPrices			 = 0x01,
	Subcommand_ExpansionRequestId	 = 0x00,
	Subcommand_ExpansionEnableOptions= 0x04,
	Subcommand_ExpansionDiagnostics	 = 0xFF,
	Subcommand_VendRequest			 = 0x00,
	Subcommand_VendCancel			 = 0x01,
	Subcommand_VendSuccess			 = 0x02,
	Subcommand_VendFailure			 = 0x03,
	Subcommand_SessionComplete		 = 0x04,
	Subcommand_CashSale				 = 0x05,
	Subcommand_OrderRequest			 = 0x11,
	Subcommand_NegativeVendRequest	 = 0x06,
	Subcommand_ReaderDisable		 = 0x00,
	Subcommand_ReaderEnable			 = 0x01,
	Subcommand_ReaderCancel			 = 0x02,
	Subcommand_RevalueRequest		 = 0x00,
	Subcommand_RevalueLimitRequest	 = 0x01
};

enum CommandId {
	CommandId_None					 = 0xFFFF,
	CommandId_Reset					 = 0x0000,
	CommandId_SetupConfig			 = 0x0100, //256
	CommandId_SetupPricesL1			 = 0x0101, //257
	CommandId_SetupPricesL3			 = 0x0102,
	CommandId_Poll					 = 0x0200,
	CommandId_VendRequest			 = 0x0300,
	CommandId_VendCancel			 = 0x0301,
	CommandId_VendSuccess			 = 0x0302,
	CommandId_VendFailure			 = 0x0303,
	CommandId_SessionComplete		 = 0x0304,
	CommandId_CashSale				 = 0x0305,
	CommandId_NegativeVendRequest	 = 0x0306,
	CommandId_OrderRequest			 = 0x0311,
	CommandId_ReaderDisable			 = 0x0400, //1024
	CommandId_ReaderEnable			 = 0x0401, //1025
	CommandId_ReaderCancel			 = 0x0402,
	CommandId_RevalueRequest		 = 0x0500, //1281
	CommandId_RevalueLimitRequest	 = 0x0501,
	CommandId_ExpansionRequestId	 = 0x0700, //1792
	CommandId_ExpansionEnableOptions = 0x0704,
	CommandId_ExpansionDiagnostics	 = 0x07FF,
};

/*
 * В ответе на POLL может быть возвращено от 1 до 16 статусов одновременно. Если нечего возвращать, то ACK.
 */
enum Status {
	Status_JustReset			 = 0x00,
	Status_ReaderConfigData		 = 0x01,
	Status_DisplayRequest		 = 0x02,
	Status_BeginSession			 = 0x03,
	Status_SessionCancelRequest	 = 0x04,
	Status_VendApproved			 = 0x05,
	Status_VendDenied			 = 0x06,
	Status_EndSession			 = 0x07,
	Status_Cancelled			 = 0x08,
	Status_PeripheralID			 = 0x09,
	Status_Error				 = 0x0A,
	Status_CmdOutOfSequence		 = 0x0B,
	Status_RevalueApproved		 = 0x0D,
	Status_RevalueDenied		 = 0x0E,
	Status_RevalueLimitAmount	 = 0x0F,
	Status_UserFileData			 = 0x10,
	Status_TimeDateRequest		 = 0x11,
	Status_DataEntryRequest		 = 0x12,
	Status_DataEntryCancel		 = 0x13,
	Status_ReqToRcv				 = 0x1B, // (Z2-Z6) The Comms Gateway is requesting to receive data from a device or VMC.
	Status_RetryDeny			 = 0x1C, // (Z2-Z3) The Comms Gateway is requesting a device or VMC to retry or deny the last FTL command.
	Status_SendBlock			 = 0x1D, // (Z2-Z34) The Comms Gateway is sending a block of data (maximum of 31 bytes) to a device or VMC.
	Status_OkToSend				 = 0x1E, // (Z2,Z3) The Comms Gateway is indicating that it is OK for a device or VMC to send it data.
	Status_ReqToSend			 = 0x1F, // (Z2-Z6) The Comms Gateway is requesting to send data to a device or VMC.
	States_OrderApproved		 = 0xFE,
	Status_Diagnostics			 = 0xFF, // (Z2-ZN) The Comms Gateway is responding to a EXPANSION/DIAGNOSTICS command.
};

/* Второй байт после Status_Error */
enum Error {
	Status_Manufacturer1_FaceIdFailed	 = 0x30,
	Status_Manufacturer1_PinCode		 = 0x33,
};

#pragma pack(push,1)
struct SetupConfigRequest {
	uint8_t command;
	uint8_t subcommand;
	uint8_t featureLevel;
	uint8_t columnsOnDisplay;
	uint8_t rowsOnDisplay;
	uint8_t displayInfo;
};

enum SetupConfigOption {
	SetupConifgOption_RefundAble = 0x01,
	SetupConfigOption_CashSale   = 0x08,
};

struct SetupConfigResponse {
	uint8_t configData;
	uint8_t featureLevel;
	LEUbcd2 currency;
	uint8_t scaleFactor;
	uint8_t decimalPlaces;
	uint8_t maxRespTime;
	uint8_t options;
};

#define MdbCashlessMaximumPrice 0xFFFF

struct SetupPricesL1Request {
	uint8_t command;
	uint8_t subcommand;
	uint16_t maximumPrice;
	uint16_t minimunPrice;
};

struct SetupPricesL3Request {
	uint8_t command;
	uint8_t subcommand;
	uint32_t maximumPrice;
	uint32_t minimunPrice;
	uint16_t currencyCode;
};

struct ExpansionRequestIdRequest {
	uint8_t command;
	uint8_t subcommand;
	uint8_t manufacturerCode[3];
	uint8_t serialNumber[12];
	uint8_t modelNumber[12];
	LEUbcd2 softwareVersion;
};

#define PerepheralId 0x09

struct ExpansionRequestIdResponseL1 {
	uint8_t perepheralId;
	uint8_t manufacturerCode[3];
	uint8_t serialNumber[12];
	uint8_t modelNumber[12];
	LEUbcd2 softwareVersion;
};

enum FeatureOption : uint32_t {
	FeatureOption_FileTransportLayer	 = 0x01,
	FeatureOption_32bitMonetaryFormat	 = 0x02,
	FeatureOption_MultiCurrency			 = 0x04,
	FeatureOption_NegativeVend			 = 0x08,
	FeatureOption_DataEntry				 = 0x10,
	FeatureOption_AlwaysIdle			 = 0x20,
	FeatureOption_Order					 = 0x80000000,
};

struct ExpansionRequestIdResponseL3 {
	uint8_t perepheralId;
	uint8_t manufacturerCode[3];
	uint8_t serialNumber[12];
	uint8_t modelNumber[12];
	LEUbcd2 softwareVersion;
	LEUint4 featureBits;
};

struct ExpansionEnableOptionsRequest {
	uint8_t command;
	uint8_t subcommand;
	LEUint4 featureBits;
};

struct BeginSession {
	uint8_t type;
	LEUint2 fundsAvailable;
};

struct RevalueRequestL2 {
	uint8_t command;
	uint8_t subcommand;
	LEUint2 amount;
};

struct RevalueRequestEC {
	uint8_t command;
	uint8_t subcommand;
	LEUint4 amount;
};

struct VendRequestRequest {
	uint8_t command;
	uint8_t subcommand;
	LEUint2 itemPrice;
	LEUint2 itemNumber;
};

struct VendRequestRequestL3 {
	uint8_t command;
	uint8_t subcommand;
	LEUint4 itemPrice;
	LEUint2 itemNumber;
//	LEUint2 itemCurrency;
};

struct VendApproved {
	uint8_t type;
	LEUint2 vendAmount;
};

struct VendSuccessRequest {
	uint8_t command;
	uint8_t subcommand;
	LEUint2 itemNumber;
};

struct OrderRequestRequest {
	uint8_t command;
	uint8_t subcommand;
	uint8_t pincodeLen;
	uint8_t pincode[8];
};

enum CashSaleNumber {
	CashSaleNumber_FreeVend		 = 0x8000,
	CashSaleNumber_TestVend		 = 0x4000,
	CashSaleNumber_NegativeVend	 = 0x2000,
	CashSaleNumber_TokenVend	 = 0x1000,
	CashSaleNumber_Mask			 = 0xFFF,
};

struct VendCashSaleRequest {
	uint8_t command;
	uint8_t subcommand;
	LEUint2 itemPrice;
	LEUint2 itemNumber;
};

struct VendCashSaleRequestEC {
	uint8_t command;
	uint8_t subcommand;
	LEUint4 itemPrice;
	LEUint2 itemNumber;
	LEUint2 currency;
};

struct PollSectionBeginSession {
	uint8_t request;
	LEUint2 amount;
};

struct PollSectionBeginSessionL3 {
	uint8_t request;
	LEUint2 amount;
	LEUint4 paymentId;
	uint8_t paymentType;
	LEUint2 paymentData;
};

struct PollSectionBeginSessionEC {
	uint8_t request;
	LEUint4 amount;
	LEUint4 paymentId;
	uint8_t paymentType;
	LEUint2 paymentData;
	uint8_t language[2];
	LEUbcd2 currency;
	uint8_t cardOptions;
};

#pragma pack(pop)

}
}

#endif
