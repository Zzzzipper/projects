#ifndef COMMON_MDB_PROTOCOL_COINCHANGER_H_
#define COMMON_MDB_PROTOCOL_COINCHANGER_H_

#include "utils/include/NetworkProtocol.h"
#include "mdb/MdbProtocol.h"

#include <stdint.h>

#define MDB_CC_TOKEN_VALUE 0xFF
#define MDB_CC_EXP_PAYOUT_POLL_MAX 200

namespace Mdb {
namespace CoinChanger {

enum Command {
	Command_Reset		= 0x00,
	Command_Setup		= 0x01,
	Command_TubeStatus	= 0x02,
	Command_Poll		= 0x03,
	Command_CoinType	= 0x04,
	Command_Dispense	= 0x05,
	Command_Expansion	= 0x07
};

enum Subcommand {
	Subcommand_None						 = 0xFF,
	Subcommand_ExpansionIdentification	 = 0x00,
	Subcommand_ExpansionFeatureEnable	 = 0x01,
	Subcommand_ExpansionPayout			 = 0x02,
	Subcommand_ExpansionPayoutStatus	 = 0x03,
	Subcommand_ExpansionPayoutValuePoll	 = 0x04,
	Subcommand_ExpansionDiagnosticStatus = 0x05,
	Subcommand_ExpansionControlledManualFillReport = 0x06,
	Subcommand_ExpansionDiagnostics		 = 0xFF
};

enum CommandId {
	CommandId_None						 = 0xFFFF,
	CommandId_Reset						 = 0x0000,
	CommandId_Setup						 = 0x0100,
	CommandId_TubeStatus				 = 0x0200,
	CommandId_Poll						 = 0x0300,
	CommandId_CoinType					 = 0x0400,
	CommandId_Dispense					 = 0x0500,
	CommandId_ExpansionIdentification	 = 0x0700,
	CommandId_ExpansionFeatureEnable	 = 0x0701,
	CommandId_ExpansionPayout			 = 0x0702,
	CommandId_ExpansionPayoutStatus		 = 0x0703,
	CommandId_ExpansionPayoutValuePoll	 = 0x0704,
	CommandId_ExpansionDiagnostics		 = 0x07FF,
};

/*
 * ¬ ответе на POLL может быть возвращено от 1 до 16 статусов одновременно. ≈сли нечего возвращать, то ACK.
 */
enum Status {
	Status_EscrowRequest			 = 0x01,
	Status_ChangerPayoutBusy		 = 0x02,
	Status_NoCredit					 = 0x03,
	Status_DefectiveTubeSensor		 = 0x04,
	Status_DoubleArrival			 = 0x05,
	Status_AcceptorUnplugged		 = 0x06,
	Status_TubeJam					 = 0x07,
	Status_RomChecksumError			 = 0x08,
	Status_CoinRoutingError			 = 0x09,
	Status_ChangerBusy				 = 0x0A,
	Status_ChangerWasReset			 = 0x0B,
	Status_CoinJam					 = 0x0C,
	Status_PossibleCreditedCoinRemoval	 = 0x0D
};

enum CoinMask {
	CoinMask_DispenseSign	= 0x80,
	CoinMask_DispenseNumber	= 0x30,
	CoinMask_DispenseCoin	= 0x0F,
	CoinMask_DepositeSign	= 0x40,
	CoinMask_DepositeRoute	= 0x30,
	CoinMask_DepositeCoin	= 0x0F
};

enum CoinRoute {
	CoinRoute_CashBox	= 0x00,
	CoinRoute_Tubes		= 0x10,
	CoinRoute_NotUsed	= 0x20,
	CoinRoute_Reject	= 0x30,
	CoinRoute_Mask		= 0x30,
};

enum Feature {
	Feature_AlternativePayout		 = 0x01,
	Feature_ExtendedDiagnostic		 = 0x02,
	Feature_ControlledFillAndPayout	 = 0x04,
	Feature_FileTransportLayer		 = 0x08
};

#pragma pack(push,1)

struct SetupResponse {
	uint8_t level;
	LEUbcd2 currency;
	uint8_t scalingFactor;
	uint8_t decimalPlaces;
	LEUint2 coinTypeRouting;
	uint8_t coins[0];
};

struct ExpansionIdentificationResponse {
	uint8_t manufacturerCode[3];
	uint8_t serialNumber[12];
	uint8_t model[12];
	LEUbcd2 softwareVersion;
	LEUbcd4 features;
};

struct ExpansionFeatureEnableRequest {
	uint8_t command;
	uint8_t subcommand;
	uint8_t enable[4];
};

struct ExpansionPayoutRequest {
	uint8_t command;
	uint8_t subcommand;
	uint8_t sum;
};

struct TubesResponse {
	LEUint2 tubeStatus;
	uint8_t tubeVolume[0];
};

struct CoinTypeRequest {
	uint8_t command;
	LEUint2 coinEnable;
	LEUint2 manualDispenseEnable;
};

struct DispenseRequest {
	uint8_t command;
	uint8_t coin;
};

#pragma pack(pop)

}
}

#endif
