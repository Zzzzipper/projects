#ifndef COMMON_CONFIG_V4_PAYMENTSTAT_H_
#define COMMON_CONFIG_V4_PAYMENTSTAT_H_

#include "mdb/master/coin_changer/MdbMasterCoinChanger.h"
#include "memory/include/Memory.h"

#pragma pack(push,1)
struct Config4PaymentStatStruct {
	//CA3 cash flow
	uint32_t cash_in_reset; // coins + bills
	uint32_t cash_in_total; // coins + bills
	uint32_t coin_in_cashbox_reset;
	uint32_t coin_in_cashbox_total;
	uint32_t coin_in_tubes_reset;
	uint32_t coin_in_tubes_total;
	uint32_t bill_in_reset;
	uint32_t bill_in_total;
	//CA4 cash out
	uint32_t coin_dispense_reset;
	uint32_t coin_dispense_total;
	uint32_t coin_manually_dispense_reset;
	uint32_t coin_manually_dispense_total;
	//CA8 cash overpay
	uint32_t cash_overpay_reset;
	uint32_t cash_overpay_total;
	//CA9 wtf?
	//CA10 tube filling
	uint32_t coin_filled_reset;
	uint32_t coin_filled_total;
	uint8_t  crc[1];
};
#pragma pack(pop)

class Config4PaymentStat {
public:
	Config4PaymentStatStruct data;

	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();
	MemoryResult reset();
	MemoryResult registerCoinIn(uint32_t value, uint8_t route);
	MemoryResult registerCoinDispense(uint32_t value);
	MemoryResult registerCoinDispenseManually(uint32_t value);
	MemoryResult registerCoinFill(uint32_t value);
	MemoryResult registerBillIn(uint32_t value);
	MemoryResult registerCashOverpay(uint32_t value);

	static uint32_t getDataSize();

private:
	Memory *memory;
	uint32_t address;
};

#endif
