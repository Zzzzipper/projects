#ifndef COMMON_CONFIG_AUTOMAT1_CONVERTER_H_
#define COMMON_CONFIG_AUTOMAT1_CONVERTER_H_

#include "Config1Automat.h"
#include "config/v3/automat/Config3Automat.h"
#include "memory/include/Memory.h"

class Config1AutomatConverter {
public:
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

private:
	Config1Automat automat1;
	Config3PriceIndexList priceIndexes2;
	Config3ProductIndexList productIndexes2;
	Config3AutomatSettings settings2;
	Config3BarcodeList barcodes2;

	void initPriceIndexes();
	void initProductIndexes();
	MemoryResult saveAutomatData(Memory *memory);
	MemoryResult saveProductList(Memory *memory);
	MemoryResult saveProducts(Memory *memory);
	MemoryResult saveProduct(Config1Product *product1, Memory *memory);
	MemoryResult savePrices(Config1PriceList *prices1, Memory *memory);
	MemoryResult savePrice(Config1Price *price, Memory *memory);
	MemoryResult saveEmptyPrice(Memory *memory);
};

#endif
