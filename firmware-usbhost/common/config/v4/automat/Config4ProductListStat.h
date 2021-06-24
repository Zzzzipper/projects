#ifndef COMMON_CONFIG_V4_PRODUCTLISTSTAT_H_
#define COMMON_CONFIG_V4_PRODUCTLISTSTAT_H_

#include "memory/include/Memory.h"

#pragma pack(push,1)
struct Config4ProductListStatStruct {
	//TA2 token
	uint32_t vend_token_num_reset;
	uint32_t vend_token_val_reset;
	uint32_t vend_token_num_total;
	uint32_t vend_token_val_total;
	//CA2 cash sale
	uint32_t vend_cash_num_reset;
	uint32_t vend_cash_val_reset;
	uint32_t vend_cash_num_total;
	uint32_t vend_cash_val_total;
	//DA2 cashless1
	uint32_t vend_cashless1_num_reset;
	uint32_t vend_cashless1_val_reset;
	uint32_t vend_cashless1_num_total;
	uint32_t vend_cashless1_val_total;
	//DB2 cashless1
	uint32_t vend_cashless2_num_reset;
	uint32_t vend_cashless2_val_reset;
	uint32_t vend_cashless2_num_total;
	uint32_t vend_cashless2_val_total;
	uint8_t  crc[1];
};
#pragma pack(pop)

class Config4ProductListStat {
public:
	Config4ProductListStatStruct data;

	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();
	MemoryResult reset();
	MemoryResult sale(const char *device, uint32_t value);

	static uint32_t getDataSize();

private:
	Memory *memory;
	uint32_t address;
};

#endif
