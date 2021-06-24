#include "Config1AutomatConverter.h"
#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

#include <cstring>

MemoryResult Config1AutomatConverter::load(Memory *memory) {
	MemoryResult result = automat1.load(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config1AutomatConverter::save(Memory *memory) {
	initPriceIndexes();
	initProductIndexes();
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
	result = productIndexes2.init(memory);
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

void Config1AutomatConverter::initPriceIndexes() {
	priceIndexes2.clear();
	priceIndexes2.add("CA", 0, Config3PriceIndexType_None);
	priceIndexes2.add("CA", 1, Config3PriceIndexType_None);
	priceIndexes2.add("DA", 0, Config3PriceIndexType_None);
	priceIndexes2.add("DA", 1, Config3PriceIndexType_None);

	Config1PriceIndexList *indexes1 = automat1.getPriceIndexList();
	for(uint16_t i = 0; i < indexes1->getSize(); i++) {
		Config1PriceIndex *price1 = indexes1->get(i);
		uint16_t index = priceIndexes2.getIndex(price1->device.get(), price1->number);
		if(index != CONFIG_INDEX_UNDEFINED) {
			Config3PriceIndex *price2 = priceIndexes2.get(index);
			price2->type = Config3PriceIndexType_Base;
		}
	}
}

void Config1AutomatConverter::initProductIndexes() {
	productIndexes2.clear();

	Config1ProductIndexList *indexes1 = automat1.getProductIndexList();
	for(uint16_t i = 0; i < indexes1->getSize(); i++) {
		Config1ProductIndex *index1 = indexes1->get(i);
		productIndexes2.add(index1->selectId.get(), index1->cashlessId);
	}
}

MemoryResult Config1AutomatConverter::saveAutomatData(Memory *memory) {
	Config3AutomatStruct data;
	data.version = CONFIG3_AUTOMAT_VERSION;
	data.automatId = automat1.getAutomatId();
	data.configId = automat1.getConfigId();
	data.paymentBus = automat1.getPaymentBus();
	data.taxSystem = automat1.getTaxSystem();
	data.decimalPoint = automat1.getDecimalPoint();
	data.maxCredit = automat1.convertMoneyToValue(500);
	data.currency = RUSSIAN_CURRENCY_RUB;
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

MemoryResult Config1AutomatConverter::saveProductList(Memory *memory) {
	Config1ProductList *list = automat1.getProducts();
	Config3ProductListData data;
	data.priceListNum = priceIndexes2.getSize();
	data.productNum = productIndexes2.getSize();
	data.totalCount = list->getTotalCount();
	data.totalMoney = list->getTotalMoney();
	data.count = list->getCount();
	data.money = list->getMoney();

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&data), sizeof(data));
}

MemoryResult Config1AutomatConverter::saveProducts(Memory *memory) {
	List<Config1Product> *list = automat1.getProducts()->getList();
	for(uint16_t i = 0; i < list->getSize(); i++) {
		Config1Product *product1 = list->get(i);
		MemoryResult result = saveProduct(product1, memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config1AutomatConverter::saveProduct(Config1Product *product1, Memory *memory) {
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

MemoryResult Config1AutomatConverter::savePrices(Config1PriceList *prices1, Memory *memory) {
	Config1PriceIndexList *priceIndexes1 = automat1.getPriceIndexList();
	for(uint16_t i = 0; i < priceIndexes2.getSize(); i++) {
		Config3PriceIndex *priceIndex2 = priceIndexes2.get(i);
		uint16_t index1 = priceIndexes1->getIndex(priceIndex2->device.get(), priceIndex2->number);
		if(index1 == 0xFFFF) {
			MemoryResult result = saveEmptyPrice(memory);
			if(result != MemoryResult_Ok) {
				return result;
			}
		} else {
			Config1Price *price1 = prices1->getByIndex(index1);
			MemoryResult result = savePrice(price1, memory);
			if(result != MemoryResult_Ok) {
				return result;
			}
		}
	}

	return MemoryResult_Ok;
}

MemoryResult Config1AutomatConverter::savePrice(Config1Price *price1, Memory *memory) {
	Config3PriceData price2;
	price2.price = price1->data.price;
	price2.totalCount = price1->data.totalCount;
	price2.totalMoney = price1->data.totalMoney;
	price2.count = price1->data.count;
	price2.money = price1->data.money;

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&price2), sizeof(price2));
}

MemoryResult Config1AutomatConverter::saveEmptyPrice(Memory *memory) {
	Config3PriceData price2;
	price2.price = 0;
	price2.totalCount = 0;
	price2.totalMoney = 0;
	price2.count = 0;
	price2.money = 0;

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc((uint8_t*)(&price2), sizeof(price2));
}
