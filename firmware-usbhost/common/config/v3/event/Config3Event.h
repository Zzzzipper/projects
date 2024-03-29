#ifndef COMMON_CONFIG_V3_EVENT_H_
#define COMMON_CONFIG_V3_EVENT_H_

#include "Config3EventData.h"
#include "fiscal_register/include/FiscalSale.h"
#include "memory/include/Memory.h"

#define CONFIG3_EVENT_UNSET 0xFFFF

class Config3Event {
public:
	enum Code : uint16_t {
		Type_None					= 0xFFFF,
		Type_OnlineStart			= 0x0000, // ����� �����������
		Type_OnlineEnd				= 0x0001, // ����� ��������
		Type_OnlineLast				= 0x0002, // ����� �������
		Type_Sale					= 0x0003, // �������
		Type_PaymentBlocked			= 0x0004, // ������� ��������� (�����������, ���� ������� ������������� ������� �����)
		Type_PaymentUnblocked		= 0x0005, // ������� ��������
		Type_PowerUp				= 0x0006, // ������� �������
		Type_PowerDown				= 0x0007, // ������� ��������
		Type_BadSignal				= 0x0008, // ������ ������
		Type_CashlessIdNotFound		= 0x0009, // �� �������� ������������ ����� �������� (STRING:<cashlessId>)
		Type_PriceListNotFound		= 0x000A, // ����� ���� �� ������ (STRING:<deviceId><priceListNumber>)
		Type_SyncConfigError		= 0x000B, // �� ������������ (�������� �������������)
		Type_PriceNotEqual			= 0x000C, // �� �������� ������������ ���� �������� (STRING:<selectId>*<expectedPrice>*<actualPrice>)
		Type_SaleDisabled			= 0x000D, // ������� ��������� ��������� ���������� �����
		Type_SaleEnabled			= 0x000E, // ������� �������� ���������
		Type_ConfigEdited			= 0x000F, // ������������ �������� ��������
		Type_ConfigLoaded			= 0x0010, // ������������ ��������� � �������
		Type_ConfigLoadFailed		= 0x0011, // ������ �������� ������������
		Type_ConfigParseFailed		= 0x0012, // ������ ������� ������������
		Type_ConfigUnknowProduct	= 0x0013, // ����������� ����� �������� (STRING:<selectId>)
		Type_ConfigUnknowPriceList	= 0x0014, // ����������� �����-���� (STRING:<deviceId><priceListNumber>)
		Type_ModemReboot			= 0x0015, // ���������� ������ (STRING:<rebootReason>)
		Type_CashCanceled			= 0x0016, // ������ ��������� �������� ���������
		Type_SaleFailed				= 0x0017, // ������ ������� (STRING:<selectId>)
		Type_WaterOverflow			= 0x0018, // ������������ ����� ������ �������
		// fiscal
		Type_FiscalUnknownError		= 0x0300, // ���������������� ������ �� (STRING:<���-������-��>)
		Type_FiscalLogicError		= 0x0301, // ����������� ��������� �� (STRING:<������-�-�����>)
		Type_FiscalConnectError		= 0x0302, // ��� ����� � ��
		Type_FiscalPassword			= 0x0303, // ������������ ������ ��
		Type_PrinterNotFound		= 0x0304, // ������� �� ������
		Type_PrinterNoPaper			= 0x0305, // � �������� ����������� ������
		Type_FiscalNotInited		= 0x0306, // ��� �� ���������������
		Type_WrongResponse			= 0x0307, // ������������ ������ ������
		Type_BrokenResponse			= 0x0308, // ������������ �����
		Type_FiscalCompleteNoData	= 0x0309, // ��� ������, �� ��������� �� ��������
		// bill validator
		Type_BillIn					= 0x0401, // ������� ������ (STRING:<nominal>)
		Type_BillUnwaitedPacket		= 0x0402, // ������ ��������������� (����������� �����)
		// coin changer
		Type_CoinIn					= 0x0501, // ������� ������ (STRING:<nominal>)
		Type_ChangeOut				= 0x0502, // ������ ����� (STRING:<sum>)
		Type_CoinUnwaitedPacket		= 0x0503, // ������ ������������� (����������� �����)
		// cashless
		Type_CashlessCanceled		= 0x0601, // ����������� ������ �������� ��������� (STRING:<selectId><timeout>)
		Type_CashlessDenied			= 0x0602, // ������ ������ ��������� ���������� (STRING:<selectId><timeout>)
		Type_SessionClosedByMaster	= 0x0603, // �������� ����������� ������ �������� ��������� (STRING:<timeout>)
		Type_SessionClosedByTimeout	= 0x0604, // �������� ����������� ������ �������� �� �������� (STRING:<timeout>)
		Type_SessionClosedByTerminal= 0x0605, // �������� ����������� ������ �������� ���������� (STRING:<timeout>)
//todo: ���� ����� ����������� ������ � ������������ � EVADTS � ���� �� ��������� ��������, ������� � 0xXX81
		// door
		Type_EGS_DoorOpen			= 0x0701, // ����� �������
		Type_EGS_DoorClose			= 0x0702, // ����� �������
		// cup system
		Type_EBJ_NoCups				= 0x0801, // ��� ��������
		// water system
		Type_EFL_NoWater			= 0x0901, // ��� ����
		Type_OBI_WasteWaterPipe		= 0x0902, // ����������� ����� �������
		// coffee brewer
		Type_EEA_CoffeeMotorFault	= 0x0A01, // ���� ��������� ������
		// hot drink system
		Type_ED_HotDrinkFault		= 0x0B01, // ���� ������� ������� ��������
		Type_EDT_Grinder			= 0x0B02, // ���� ���������
		// debug
		Type_EventReadError			= 0xFF01, // ������ ������ ������� �������
		Type_WatchDog				= 0xFF02, // �������� WatchDog
		Type_MdbUnwaitedPacket		= 0xFF03, // ����������� ����� � MDB-���������
		Type_DtsUnwaitedEvent		= 0xFF04,
	};

	Config3Event();
	void bind(Memory *memory);
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	void set(DateTime *datetime, uint16_t code, const char *str = "");
	void set(DateTime *datetime, Fiscal::Sale *sale, uint16_t index);
	void set(Config3EventStruct *data);
	DateTime *getDate() { return &data.date; }
	uint16_t getCode() { return data.code; }
	uint32_t getNumber() { return data.data.number; }
	const char *getString() { return data.data.string.get(); }
	Config3EventSale *getSale() { return &(data.sale); }
	Config3EventStruct *getData() { return &data; }

	static uint32_t getDataSize();
	static const char *getEventName(Config3Event *event);
	static void getEventDescription(Config3Event *event, StringBuilder *buf);

private:
	Memory *memory;
	uint32_t address;
	Config3EventStruct data;

	static void getEventSaleDescription(Config3Event *event, StringBuilder *buf);
	static void getEventPriceNotEqualDescription(Config3Event *event, StringBuilder *buf);
	static const char *paymentDeviceToString(const char *device);
};

#endif
