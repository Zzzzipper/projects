#include "Config2AutomatConverter.h"
#include "config/v2/automat/Config2AutomatData.h"
#include "logger/include/Logger.h"

#include <string.h>

MemoryResult Config2AutomatConverter::load(Memory *memory) {
	MemoryResult result = automat1.load(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config2AutomatConverter::save(Memory *memory) {
	initPriceIndexes();
	MemoryResult result = saveAutomatData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save automat data failed");
		return result;
	}
	result = saveProductList(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save product list data failed");
		return result;
	}
	result = saveProducts(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save product data failed");
		return result;
	}
	result = automat1.getProductIndexList()->init(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save product index failed");
		return result;
	}
	result = priceIndexes2.init(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save price index failed");
		return result;
	}
	result = settings2.init(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save settings failed");
		return result;
	}
	result = barcodes2.init(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Save barcodes failed");
		return result;
	}
	return MemoryResult_Ok;
}

void Config2AutomatConverter::initPriceIndexes() {
	priceIndexes2.clear();
	priceIndexes2.add("CA", 0, Config3PriceIndexType_None);
	priceIndexes2.add("CA", 1, Config3PriceIndexType_None);
	priceIndexes2.add("DA", 0, Config3PriceIndexType_None);
	priceIndexes2.add("DA", 1, Config3PriceIndexType_None);

	Config3PriceIndexList *indexes1 = automat1.getPriceIndexList();
	for(uint16_t i = 0; i < indexes1->getSize(); i++) {
		Config3PriceIndex *index1 = indexes1->get(i);
		uint16_t index = priceIndexes2.getIndex(index1->device.get(), index1->number);
		if(index != CONFIG_INDEX_UNDEFINED) {
			Config3PriceIndex *index2 = priceIndexes2.get(index);
			index2->type = index1->type;
			index2->timeTable.set(&index1->timeTable);
		}
	}
}

MemoryResult Config2AutomatConverter::saveAutomatData(Memory *memory) {
	Config3AutomatStruct data;
	data.version = CONFIG3_AUTOMAT_VERSION;
	data.automatId = automat1.getAutomatId();
	data.configId = automat1.getConfigId();
	data.paymentBus = automat1.getPaymentBus();
	data.taxSystem = automat1.getTaxSystem();
	data.decimalPoint = automat1.getDecimalPoint();
	data.maxCredit = automat1.convertMoneyToValue(500);
	data.currency = automat1.getCurrency();
	data.flags = 0;
	data.ext1Device = 0;
	data.freeSale = false;
	data.cashlessNum = 2;
	data.scaleFactor = 1;
	data.internetDevice = Config3Automat::InternetDevice_Gsm;
	data.ext2Device = 0;
	data.usb1Device = 0;
	data.qrType = 0;
	data.bvOn = 1;
	memset(data.reserved, 0, sizeof(data.reserved));

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&data), sizeof(data));
}

MemoryResult Config2AutomatConverter::saveProductList(Memory *memory) {
	Config2ProductList *list = automat1.getProductList();
	Config2ProductListData data;
	data.priceListNum = priceIndexes2.getSize();
	data.productNum = automat1.getProductIndexList()->getSize();
	data.totalCount = list->getTotalCount();
	data.totalMoney = list->getTotalMoney();
	data.count = list->getCount();
	data.money = list->getMoney();

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&data), sizeof(data));
}

MemoryResult Config2AutomatConverter::saveProducts(Memory *memory) {
	Config2ProductList *products = automat1.getProductList();
	for(uint16_t i = 0; i < products->getLen(); i++) {
		Config2Product *product = products->getByIndex(i);
		MemoryResult result = saveProduct(product, memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config2AutomatConverter::saveProduct(Config2Product *product1, Memory *memory) {
	Config3ProductData product2;
	product2.enable = true;
	product2.wareId = 0;
	product2.name.set(product1->data.name.get());
	product2.taxRate = product1->data.taxRate;
	product2.totalCount = product1->data.totalCount;
	product2.totalMoney = product1->data.totalMoney;
	product2.count = product1->data.count;
	product2.money = product1->data.money;
	product2.testTotalCount = 0;
	product2.testCount = 0;
	product2.freeTotalCount = 0;
	product2.freeCount = 0;

	MemoryCrc crc(memory);
	MemoryResult result = crc.writeDataWithCrc((uint8_t*)(&product2), sizeof(product2));
	if(result != MemoryResult_Ok) {
		return result;
	}

	return savePrices(&(product1->prices), memory);
}

MemoryResult Config2AutomatConverter::savePrices(Config2PriceList *prices1, Memory *memory) {
	Config3PriceIndexList *priceIndexes1 = automat1.getPriceIndexList();
	for(uint16_t i = 0; i < priceIndexes2.getSize(); i++) {
		Config3PriceIndex *index2 = priceIndexes2.get(i);
		uint16_t index1 = priceIndexes1->getIndex(index2->device.get(), index2->number);
		if(index1 == 0xFFFF) {
			MemoryResult result = saveEmptyPrice(memory);
			if(result != MemoryResult_Ok) {
				return result;
			}
		} else {
			Config2Price *price1 = prices1->getByIndex(index1);
			MemoryResult result = savePrice(price1, memory);
			if(result != MemoryResult_Ok) {
				return result;
			}
		}
	}

	return MemoryResult_Ok;
}

MemoryResult Config2AutomatConverter::savePrice(Config2Price *price1, Memory *memory) {
	Config2PriceData price2;
	price2.price = price1->data.price;
	price2.totalCount = price1->data.totalCount;
	price2.totalMoney = price1->data.totalMoney;
	price2.count = price1->data.count;
	price2.money = price1->data.money;

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&price2), sizeof(price2));
}

MemoryResult Config2AutomatConverter::saveEmptyPrice(Memory *memory) {
	Config2PriceData price2;
	price2.price = 0;
	price2.totalCount = 0;
	price2.totalMoney = 0;
	price2.count = 0;
	price2.money = 0;

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&price2), sizeof(price2));
}
