#ifndef EXE_PROTOCOL_H_
#define EXE_PROTOCOL_H_

namespace Exe {

#define EXE_PACKET_MAX_SIZE 8
#define EXE_POLL_TIMEOUT 100
#define EXE_COMMAND_TIMEOUT 100
#define EXE_DATA_TIMEOUT 1000
#define EXE_APPROVE_TIMEOUT 180000
#define EXE_VEND_TIMEOUT 180000
#define EXE_SEND_TIMEOUT 1
#define EXE_DATA_SIZE 10
#define EXE_SHOW_TIMEOUT 1500
#define EXE_SHOW_DELAY_COUNT EXE_SHOW_TIMEOUT/EXE_POLL_TIMEOUT

enum Control {
	Control_ACK	 = 0x00,
	Control_PNAK = 0xFF,
};

enum Mask {
	Mask_Device		 = 0xE0,
	Flag_Command	 = 0x10,
	Mask_Data		 = 0x0F,
	Flag_ExactChange = 0x01, // равен 1, если автомат должен выводить на экран "нет сдачи, внесите точную сумму"
};

enum Device {
	Device_VMC			= 0x20,
	Device_AuditUnit	= 0x40,
	Device_PaymentUnit	= 0x60,
};

enum DecimalPoint {
	DecimalPoint_0 = 0x01,
	DecimalPoint_1 = 0x02,
	DecimalPoint_2 = 0x04,
	DecimalPoint_3 = 0x08,
};

enum Command {
	Command_Status				= 0x01, // 0x31
	Command_Credit				= 0x02, // 0x32
	Command_Vend				= 0x03, // 0x33
	Command_Audit				= 0x04, // 0x34
	Command_SendAuditAddress	= 0x05, // 0x34
	Command_SendAuditData		= 0x06, // 0x36
	Command_Identify			= 0x07, // 0x37
	Command_AcceptData			= 0x08, // 0x38
	Command_DataSync			= 0x09, // 0x39
	Command_NegativeAcknowledge	= 0x0F
};

enum ResponseCode {
	ResponseCode_OK				= 0,
	ResponseCode_FailureCommand	= 251,
	ResponseCode_LostSync		= 254,
	ResponseCode_Negative		= 255,
};

enum CommandStatus {
	CommandStatus_VendingInhibited = 0x40,
	CommandStatus_FreeVend		= 0x80,
};

enum CommandCredit {
	CommandCredit_CreditMax		= 0xFA,
	CommandCredit_NoVendRequest	= 0xFE,
};

enum CommandVend {
	CommandVend_ResultFlag		= 0x80,
};

}

#endif
