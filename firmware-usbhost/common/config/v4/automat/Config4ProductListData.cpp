#include "Config4ProductListData.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

MemoryResult Config4ProductListData::init(uint16_t productNum, uint16_t priceListNum, Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
#if 0
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
#endif
	data.productNum = productNum;
	data.priceListNum = priceListNum;
	return save();
}

MemoryResult Config4ProductListData::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	LOG_DEBUG(LOG_CFG, "productNum=" << data.productNum);
	LOG_DEBUG(LOG_CFG, "priceListNum=" << data.priceListNum);
#if 0
	LOG_DEBUG(LOG_CFG, "totalCount=" << data.totalCount);
	LOG_DEBUG(LOG_CFG, "totalMoney=" << data.totalMoney);
	LOG_DEBUG(LOG_CFG, "count=" << data.count);
	LOG_DEBUG(LOG_CFG, "money=" << data.money);
#endif
	return MemoryResult_Ok;
}

MemoryResult Config4ProductListData::save() {
	LOG_DEBUG(LOG_CFG, "saveData " << address);
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

uint32_t Config4ProductListData::getDataSize() {
	return sizeof(Config4ProductListDataStruct);
}
