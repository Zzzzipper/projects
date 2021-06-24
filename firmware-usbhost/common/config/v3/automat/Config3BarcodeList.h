#ifndef COMMON_CONFIG_BARCODELIST_H_
#define COMMON_CONFIG_BARCODELIST_H_

#include "memory/include/Memory.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/List.h"

class Config3Barcode {
public:
	uint32_t wareId;
	StringBuilder value;

	Config3Barcode();
};

class Config3BarcodeList {
public:
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	void add(uint32_t wareId, const char *barcode);
	uint32_t getWareIdByBarcode(const char *barcode);
	const char *getBarcodeByWareId(uint32_t wareId, uint32_t index);
	void clear();

private:
	Memory *memory;
	uint32_t address;
	uint32_t num;
	List<Config3Barcode> list;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData();
	MemoryResult loadBarcodeData(Config3Barcode *entry);
	MemoryResult saveBarcodeData(Config3Barcode *entry);
};

#endif
