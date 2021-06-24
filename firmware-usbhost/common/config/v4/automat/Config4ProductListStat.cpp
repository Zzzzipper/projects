#include "Config4ProductListStat.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <memory.h>
#include <string.h>

MemoryResult Config4ProductListStat::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	memset(&data, 0, sizeof(data));
	return save();
}

MemoryResult Config4ProductListStat::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config4ProductListStat::save() {
	LOG_DEBUG(LOG_CFG, "saveData " << address);
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config4ProductListStat::reset() {
	LOG_DEBUG(LOG_CFG, "reset");
	data.vend_token_num_reset = 0;
	data.vend_token_val_reset = 0;
	data.vend_cash_num_reset = 0;
	data.vend_cash_val_reset = 0;
	data.vend_cashless1_num_reset = 0;
	data.vend_cashless1_val_reset = 0;
	data.vend_cashless2_num_reset = 0;
	data.vend_cashless2_val_reset = 0;
	return save();
}

MemoryResult Config4ProductListStat::sale(const char *device, uint32_t value) {
	if(strcmp(device, "TA") == 0) {
		data.vend_token_num_total += 1;
		data.vend_token_val_total += value;
		data.vend_token_num_reset += 1;
		data.vend_token_val_reset += value;
		return save();
	} else if(strcmp(device, "CA") == 0) {
		data.vend_cash_num_total += 1;
		data.vend_cash_val_total += value;
		data.vend_cash_num_reset += 1;
		data.vend_cash_val_reset += value;
		return save();
	} else if(strcmp(device, "DA") == 0) {
		data.vend_cashless1_num_total += 1;
		data.vend_cashless1_val_total += value;
		data.vend_cashless1_num_reset += 1;
		data.vend_cashless1_val_reset += value;
		return save();
	} else if(strcmp(device, "DB") == 0) {
		data.vend_cashless2_num_total += 1;
		data.vend_cashless2_val_total += value;
		data.vend_cashless2_num_reset += 1;
		data.vend_cashless2_val_reset += value;
		return save();
	} else {
		return MemoryResult_Ok;
	}
}

uint32_t Config4ProductListStat::getDataSize() {
	return sizeof(Config4ProductListStatStruct);
}
