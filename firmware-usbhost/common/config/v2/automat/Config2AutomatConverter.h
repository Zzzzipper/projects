#ifndef COMMON_CONFIG_AUTOMAT2_CONVERTER_H_
#define COMMON_CONFIG_AUTOMAT2_CONVERTER_H_

#include "Config2Automat.h"
#include "config/v3/automat/Config3Automat.h"
#include "memory/include/Memory.h"

class Config2AutomatConverter {
public:
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

private:
	Config2Automat automat1;
	Config3PriceIndexList priceIndexes2;
	Config3AutomatSettings settings2;
	Config3BarcodeList barcodes2;

	void initPriceIndexes();
	MemoryResult saveAutomatData(Memory *memory);
	MemoryResult saveProductList(Memory *memory);
	MemoryResult saveProducts(Memory *memory);
	MemoryResult saveProduct(Config2Product *product1, Memory *memory);
	MemoryResult savePrices(Config2PriceList *prices1, Memory *memory);
	MemoryResult savePrice(Config2Price *price, Memory *memory);
	MemoryResult saveEmptyPrice(Memory *memory);
};

#endif
