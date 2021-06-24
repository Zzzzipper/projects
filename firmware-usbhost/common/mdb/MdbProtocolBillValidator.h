#ifndef COMMON_MDB_PROTOCOLBILLVALIDATOR_H_
#define COMMON_MDB_PROTOCOLBILLVALIDATOR_H_

#include "utils/include/NetworkProtocol.h"
#include "mdb/MdbProtocol.h"

namespace Mdb {
namespace BillValidator {

enum Command {
	Command_Reset		= 0x00,
	Command_Setup		= 0x01,
	Command_Security	= 0x02,
	Command_Poll		= 0x03,
	Command_BillType	= 0x04,
	Command_Escrow		= 0x05,
	Command_Stacker		= 0x06,
	Command_Expansion	= 0x07
};

enum Subcommand {
	Subcommand_None							 = 0xFF,
	Subcommand_ExpansionIdentificationL1	 = 0x00,
	Subcommand_ExpansionFeatureEnable		 = 0x01,
	Subcommand_ExpansionIdentificationL2	 = 0x02,
	Subcommand_ExpansionRecyclerSetup		 = 0x03,
	Subcommand_ExpansionRecyclerEnable		 = 0x04,
	Subcommand_ExpansionBillDispenseStatus	 = 0x05,
};

enum CommandId {
	CommandId_None							 = 0xFFFF,
	CommandId_Reset							 = 0x0000,
	CommandId_Setup							 = 0x0100,
	CommandId_Security						 = 0x0200,
	CommandId_Poll							 = 0x0300,
	CommandId_BillType						 = 0x0400,
	CommandId_Escrow						 = 0x0500,
	CommandId_Stacker						 = 0x0600,
	CommandId_ExpansionIdentificationL1		 = 0x0700,
};

enum Status {
	Status_DefectiveMotor              = 0x01, // One of the motors has failed to perform its expected assignment.
	Status_SensorProblem               = 0x02, // One of the sensors has failed to provide its response.
	Status_ValidatorBusy               = 0x03, // The validator is busy and can not answer a detailed command right now
	Status_RomChecksumError            = 0x04, // The validators internal checksum does not match the calculated checksum.
	Status_ValidatorJammed             = 0x05, // A bill(s) has jammed in the acceptance path.
	Status_JustReset                   = 0x06, // The validator has been reset since the last POLL.
	Status_BillRemoved                 = 0x07, // A bill in the escrow position has been removed by an unknown means. A BILL RETURNED message should also be sent.
	Status_CashBoxOutOfPosition        = 0x08, // The validator has detected the cash box to be open or removed.
	Status_ValidatorDisabled           = 0x09, // The validator has been disabled, by the VMC or because of internal conditions.
	Status_InvalidEscrowRequest        = 0x0A, // An ESCROW command was requested for a bill not in the escrow position.
	Status_BillRejected                = 0x0B, // A bill was detected, but rejected because it could not be identified.
	Status_PossibleCreditedBillRemoval = 0x0C, // There has been an attempt to remove a credited (stacked) bill.
	Status_InputTriesWhileDisabled     = 0x40, // (010xxxxx) Number of attempts to input a bill while validator is disabled.
};

enum BillMask {
	BillMask_Sign	= 0x80,
	BillMask_Route	= 0x70,
	BillMask_Type	= 0x0F
};

enum BillEvent {
	BillEvent_BillStacked			= 0x00,
	BillEvent_EscrowPosition		= 0x10,
	BillEvent_BillReturned			= 0x20,
	BillEvent_BillToRecycler		= 0x30,
	BillEvent_DisabledBillRejected	= 0x40,
	BillEvent_BillToRecyclerManual	= 0x50,
	BillEvent_ManualDispense		= 0x60,
	BillEvent_BillToCashbox			= 0x70
};

#pragma pack(push, 1)
struct SetupResponse {
	uint8_t level;
	LEUbcd2 currency;
	LEUint2 scalingFactor;
	uint8_t decimalPlaces;
	LEUint2 stackerCapacity;
	LEUint2 securityLevel;
	uint8_t escrow;
	uint8_t bills[0];
};

struct ExpansionIdentificationL1Response {
	uint8_t manufacturerCode[MDB_MANUFACTURER_SIZE];
	uint8_t serialNumber[MDB_SERIAL_NUMBER_SIZE];
	uint8_t model[MDB_MODEL_SIZE];
	LEUbcd2 softwareVersion;
};

struct SecurityRequest {
	uint8_t command;
	LEUint2 security;
};

struct BillTypeRequest {
	uint8_t command;
	LEUint2 billEnable;
	LEUint2 billEscrowEnable;
};

struct EscrowRequest {
	uint8_t command;
	uint8_t escrowStatus;
};

#define BV_STACKER_FULL_FLAG 0x8000
#define BV_STACKER_NUM_MASK 0x7FFF

struct StackerResponse {
	LEUint2 billNum;
};

#pragma pack(pop)

}
}

#endif
