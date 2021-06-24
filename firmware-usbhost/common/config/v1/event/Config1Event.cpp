#include "Config1EventList.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

#include <string.h>
#include <strings.h>

void ConfigEventSale1::set(ConfigEventSale1 *data) {
	selectId.set(data->selectId.get());
	name.set(data->name.get());
	device.set(data->device.get());
	priceList = data->priceList;
	price = data->price;
	fiscalRegister = data->fiscalRegister;
	fiscalStorage = data->fiscalStorage;
	fiscalDocument = data->fiscalDocument;
	fiscalSign = data->fiscalSign;
}

Config1Event::Config1Event() {

}

MemoryResult Config1Event::load(Memory *memory) {
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.read(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.readCrc();
}

MemoryResult Config1Event::save(Memory *memory) {
	if(memory == NULL) {
		LOG_ERROR(LOG_CFG, "Memory not inited");
		return MemoryResult_ProgError;
	}
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.write(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.writeCrc();
}
