#include "Config3BarcodeList.h"

#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "logger/include/Logger.h"

#pragma pack(push,1)
struct Config3BarcodeListData {
	uint16_t barcodeNum;
	uint8_t  crc[1];
};

struct Config3BarcodeData {
	uint32_t wareId;
	StrParam<32> name;
	uint8_t  crc[1];
};
#pragma pack(pop)

Config3Barcode::Config3Barcode() : value(32, 32) {}

MemoryResult Config3BarcodeList::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	list.clearAndFree();
	return saveData();
}

MemoryResult Config3BarcodeList::load(Memory *memory) {
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Barcode header load failed");
		return result;
	}
	for(uint8_t i = 0; i < num; i++) {
		Config3Barcode *barcode = new Config3Barcode;
		result = loadBarcodeData(barcode);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Barcode " << i << " read failed");
			return result;
		}
		list.add(barcode);
		LOG_INFO(LOG_CFG, "load barcode " << i << "/" << barcode->wareId << "/" << barcode->value.getString());
	}
	return MemoryResult_Ok;
}

MemoryResult Config3BarcodeList::save() {
	MemoryResult result = saveData();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Barcode header load failed");
		return result;
	}
	for(uint8_t i = 0; i < list.getSize(); i++) {
		Config3Barcode *barcode = list.get(i);
		if(barcode == NULL) {
			return MemoryResult_ProgError;
		}
		result = saveBarcodeData(barcode);
		if(result != MemoryResult_Ok) {
			return result;
		}
		LOG_INFO(LOG_CFG, "save barcode " << i << "/" << barcode->wareId << "/" << barcode->value.getString());
	}
	return MemoryResult_Ok;
}

void Config3BarcodeList::add(uint32_t wareId, const char *barcode) {
	Config3Barcode *entry = new Config3Barcode;
	entry->wareId = wareId;
	entry->value.set(barcode);
	list.add(entry);
}

uint32_t Config3BarcodeList::getWareIdByBarcode(const char *barcode) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3Barcode *entry = list.get(i);
		if(entry == NULL) {
			return 0;
		}
		if(entry->value == barcode) {
			return entry->wareId;
		}
	}
	return 0;
}

const char *Config3BarcodeList::getBarcodeByWareId(uint32_t wareId, uint32_t index) {
	uint32_t count = 0;
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3Barcode *entry = list.get(i);
		if(entry == NULL) {
			return NULL;
		}
		if(entry->wareId == wareId) {
			if(count == index) {
				return entry->value.getString();
			} else {
				count++;
			}
		}
	}
	return NULL;
}

void Config3BarcodeList::clear() {
	list.clearAndFree();
}

MemoryResult Config3BarcodeList::loadData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	Config3BarcodeListData data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	num = data.barcodeNum;
	LOG_DEBUG(LOG_CFG, "BarcodeNum=" << num);
	return MemoryResult_Ok;
}

MemoryResult Config3BarcodeList::saveData() {
	LOG_DEBUG(LOG_CFG, "saveData " << address << "," << list.getSize());
	Config3BarcodeListData data;
	data.barcodeNum = list.getSize();
	MemoryCrc crc(memory);
	MemoryResult result = crc.writeDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3BarcodeList::loadBarcodeData(Config3Barcode *entry) {
	LOG_DEBUG(LOG_CFG, "loadBarcodeData");
	Config3BarcodeData data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	entry->wareId = data.wareId;
	entry->value.set(data.name.get());
	return MemoryResult_Ok;
}

MemoryResult Config3BarcodeList::saveBarcodeData(Config3Barcode *entry) {
	LOG_DEBUG(LOG_CFG, "saveBarcodeData");
	Config3BarcodeData data;
	data.wareId = entry->wareId;
	data.name.set(entry->value.getString());
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}
