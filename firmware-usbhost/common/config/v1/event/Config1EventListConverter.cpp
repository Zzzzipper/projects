#include "Config1EventListConverter.h"
#include "config/v3/event/Config3EventData.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

MemoryResult Config1EventListConverter::load(Memory *memory) {
	this->memory = memory;
	MemoryResult result = events1.load(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config1EventListConverter::save(Memory *memory) {
	MemoryResult result = saveEventList(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save head data failed");
		return result;
	}
	result = saveEvents(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save product data failed");
		return result;
	}
	result = saveEventList(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save tail data failed");
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config1EventListConverter::saveEventList(Memory *memory) {
	Config3EventListData data;
	data.version = CONFIG_EVENTLIST_VERSION;
	data.size = events1.getSize();
	data.first = events1.getFirst();
	data.last = events1.getLast();
	data.sync = events1.getSync();

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&data), sizeof(data));
}

MemoryResult Config1EventListConverter::saveEvents(Memory *memory) {
	for(uint16_t i = 0; i < events1.getSize(); i++) {
		Config1Event *event1 = events1.getByIndex(i);
		MemoryResult result = saveEvent(event1, memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config1EventListConverter::saveEvent(Config1Event *event1, Memory *memory) {
	Config3EventStruct event2;
	event2.date.set(event1->getDate());
	event2.code = event1->getCode();
	if(event2.code == Config1Event::Type_Sale) {
		ConfigEventStruct1 *data1 = event1->getData();
		event2.sale.selectId.set(data1->sale.selectId.get());
		event2.sale.wareId = 0;
		event2.sale.name.set(data1->sale.name.get());
		event2.sale.device.set(data1->sale.device.get());
		event2.sale.priceList = data1->sale.priceList;
		event2.sale.price = data1->sale.price;
		event2.sale.taxRate = 0;
		event2.sale.taxValue = 0;
		event2.sale.fiscalRegister = 0;
		event2.sale.fiscalStorage = 0;
		event2.sale.fiscalDocument = 0;
		event2.sale.fiscalSign = 0;
	} else {
		event2.data.number = event1->getNumber();
		event2.data.string.set(event1->getString());
	}

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&event2), sizeof(event2));
}
