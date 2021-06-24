#ifndef COMMON_MDB_PROTOCOL_COM_GATEWAY_H_
#define COMMON_MDB_PROTOCOL_COM_GATEWAY_H_

#include "utils/include/NetworkProtocol.h"
#include "mdb/MdbProtocol.h"

#include <stdint.h>

/*
1B024547535F3000000000002020021812280000000001

1B
02 - Event
code(4547535F300000000000)
date(20200218)
time(1228)
0000000001


Type = 01,
 Transaction (1)
 Transaction Type (1)
 Selection (Row/Col.) (2)
 Price (2) Cash in,
 Coin tubes (2) Cash in,
 Cashbox (2) Cash in,
 Bills (2) Value in,
 Cashless #1 (2) Value in,
 Cashless #2 (2)
 Revalue to Cashless #1 (2)
 Revalue to Cashless #2 (2)
 Cash out (2)
 Discount Amount (2)
 Surcharge Amount (2)
 User Group # (1)
 Price List (1)
 Date (4)
 Time (2)

1B
Transaction(01)
Transaction Type(07)
Selection(0002)
Price(012C)
Coin tubes(0000)
Cashbox(0000)
Bills(0000)
Cashless #1(0000)
Cashless #2(0000)
Revalue to Cashless #1(0000)
Revalue to Cashless #2(0000)
Cash out(0000)
Discount Amount(0000)
Surcharge Amount(0000)
User Group #(00)
Price List(02)
Date(20200218)
Time(1308)
 */

namespace Mdb {
namespace ComGateway {

enum Command {
	Command_Reset		= 0x00,
	Command_Setup		= 0x01,
	Command_Poll		= 0x02,
	Command_Report		= 0x03,
	Command_Control		= 0x04,
	Command_Expansion	= 0x07,
};

enum Subcommand {
	Subcommand_None						 = 0xFF,
	Subcommand_ExpansionIdentification	 = 0x00,
	Subcommand_ExpansionFeatureEnable	 = 0x01,
	Subcommand_ReportTransaction		 = 0x01,
	Subcommand_ReportDtsEvent			 = 0x02,
	Subcommand_ReportAssetId			 = 0x03,
	Subcommand_ReportCurrencyId			 = 0x04,
	Subcommand_ReportProductId			 = 0x05,
	Subcommand_ControlDisabled			 = 0x00,
	Subcommand_ControlEnabled			 = 0x01,
	Subcommand_ControlTransmit			 = 0x02,
};

enum CommandId {
	CommandId_None						 = 0xFFFF,
	CommandId_Reset						 = 0x0000,
	CommandId_Setup						 = 0x0100,
	CommandId_Poll						 = 0x0200,
	CommandId_Report					 = 0x0300,
	CommandId_ReportTransaction			 = 0x0301,
	CommandId_ReportDtsEvent			 = 0x0302,
	CommandId_ReportAssetId				 = 0x0303,
	CommandId_ReportCurrencyId			 = 0x0304,
	CommandId_ReportProductId			 = 0x0305,
	CommandId_ControlDisabled			 = 0x0400,
	CommandId_ControlEnabled			 = 0x0401,
	CommandId_ControlTransmit			 = 0x0402,
	CommandId_ExpansionIdentification	 = 0x0700,
	CommandId_ExpansionFeatureEnable	 = 0x0701,
};

/*
 * ¬ ответе на POLL может быть возвращено от 1 до 16 статусов одновременно. ≈сли нечего возвращать, то ACK.
 */
enum Status {
	Status_JustReset			 = 0x00,
	Status_Configuration		 = 0x01,
	Status_RequestToTransmit	 = 0x02,
	Status_DataTransmited		 = 0x03,
	Status_Error				 = 0x04,
	Status_DtsEventAck			 = 0x05,
	Status_PeripheralId			 = 0x06,
	Status_RadioSignalStrength	 = 0x07,
	Status_Diagnostics			 = 0xFF, // (Z2-ZN) The Comms Gateway is responding to a EXPANSION/DIAGNOSTICS command.*/
};

#pragma pack(push,1)
struct SetupRequest {
	uint8_t command;
	uint8_t featureLevel;
	uint8_t scaleFactor;
	uint8_t decimalPoint;
};

struct SetupResponse {
	uint8_t command;
	uint8_t featureLevel;
	LEUint2 maxResponseTime;
};

#define ComGatewayPerepheralId 0x06

enum ExpansionOption {
	ExpansionOption_FileTransport = 0x01,
	ExpansionOption_VerboseMode = 0x02,
};

struct ExpansionIdentificationResponse {
	uint8_t perepheralId;
	uint8_t manufacturerCode[3];
	uint8_t serialNumber[12];
	uint8_t modelNumber[12];
	LEUbcd2 softwareVersion;
	LEUint4 featureBits;
};

struct ExpanstionFeatureEnbaleRequest {
	uint8_t command;
	uint8_t subcommand;
	LEUint4 featureBits;
};

enum TransactionType {
	TransactionType_PaidVend 	 = 0x01,
	TransactionType_TokenVend	 = 0x02,
	TransactionType_FreeVend	 = 0x03,
	TransactionType_TestVend	 = 0x04,
	TransactionType_Revalue		 = 0x05,
	TransactionType_NegativeVend = 0x06,
	TransactionType_Vendless	 = 0x07,
	TransactionType_Service		 = 0x08,
};

struct ReportTransactionRequest {
	uint8_t command;
	uint8_t subcommand;
	uint8_t transactionType;
	LEUint2 itemNumber;
	LEUint2 price;
	LEUint2 cashInCoinTubes;
	LEUint2 cashInCashbox;
	LEUint2 cashInBills;
	LEUint2 valueInCashless1;
	LEUint2 valueInCashless2;
	LEUint2 revalueInCashless1;
	LEUint2 revalueInCashless2;
	LEUint2 cashOut;
	LEUint2 discountAmount;
	LEUint2 surchargeAmount;
	uint8_t userGroup;
	uint8_t priceList;
	uint8_t date[4];
	uint8_t time[2];
};

struct ReportTransactionData {
	uint8_t transactionType;
	uint16_t itemNumber;
	uint32_t price;
	uint32_t cashInCoinTubes;
	uint32_t cashInCashbox;
	uint32_t cashInBills;
	uint32_t valueInCashless1;
	uint32_t valueInCashless2;
	uint32_t revalueInCashless1;
	uint32_t revalueInCashless2;
	uint32_t cashOut;
	uint32_t discountAmount;
	uint32_t surchargeAmount;
	uint8_t userGroup;
	uint8_t priceList;

	ReportTransactionData() {
		transactionType = 0;
		itemNumber = 0;
		price = 0;
		cashInCoinTubes = 0;
		cashInCashbox = 0;
		cashInBills = 0;
		valueInCashless1 = 0;
		valueInCashless2 = 0;
		revalueInCashless1 = 0;
		revalueInCashless2 = 0;
		cashOut = 0;
		discountAmount = 0;
		surchargeAmount = 0;
		userGroup = 0;
		priceList = 0;
	}
};

struct ReportDtsEventRequest {
	uint8_t command;
	uint8_t subcommand;
	uint8_t code[10];
	LEUbcd2	year;
	Ubcd1   month;
	Ubcd1   day;
	Ubcd1   hour;
	Ubcd1   minute;
	LEUint4 duration; // The duration of the event in total minutes.
	uint8_t activity;
};

struct ReportEventData {
	StrParam<10> code;
	DateTime datetime;
	uint32_t duration;
	uint8_t activity;
};

#pragma pack(pop)

}
}

#endif
