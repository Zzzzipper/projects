#include "Config1Automat.h"
#include "Evadts1Protocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Number.h"
#include "utils/include/DecimalPoint.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CONFIG_AUTOMAT_VERSION1 1
#define PRODUCT_ID_SIZE 6 // from EVADTS, Appendix A
#define PRODUCT_NAME_SIZE 20 // from EVADTS, Appendix A

#pragma pack(push,1)
struct Config1AutomatStruct {
	uint8_t version;
	uint32_t automatId;
	uint32_t configId;
	uint16_t paymentBus;
	uint16_t taxSystem;
	uint16_t decimalPoint;
	uint32_t devider;
};
#pragma pack(pop)

Config1Automat::Config1Automat() {
	setDefault();
}

void Config1Automat::setDefault() {
	setConfigId(0);
	setAutomatId(0);
	setPaymentBus(PaymentBus_MdbMaster);
	setDecimalPoint(DECIMAL_POINT_DEFAULT);
}

void Config1Automat::init() {
	for(uint16_t i = 0; i < productIndexes.getSize(); i++) {
		Config1Product *product = new Config1Product;
		product->setDefault();
		for(uint16_t j = 0; j < priceIndexes.getSize(); j++) {
			Config1Price *price = new Config1Price;
			price->setDefault();
			product->prices.add(price);
		}
		products.add(product);
	}
	products.set(priceIndexes.getSize(), productIndexes.getSize());
}

MemoryResult Config1Automat::load(Memory *memory) {
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat load failed");
		return result;
	}
	result = products.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Products load failed");
		return result;
	}
	result = productIndexes.load(products.getProductNum(), memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Product indexes load failed");
		return result;
	}
	result = priceIndexes.load(products.getPriceListNum(), memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Price indexes load failed");
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config1Automat::save(Memory *memory) {
	MemoryResult result = saveData(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = products.save(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = productIndexes.save(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = priceIndexes.save(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	LOG_INFO(LOG_CFG, "Automat config saved");
	return MemoryResult_Ok;
}

MemoryResult Config1Automat::loadData(Memory *memory) {
	Config1AutomatStruct data;
	MemoryCrc crc(memory);
	crc.startCrc();
	crc.read(&data, sizeof(data));
	MemoryResult result = crc.readCrc();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong automat config CRC");
		return result;
	}

	if(data.version != CONFIG_AUTOMAT_VERSION1) {
		LOG_ERROR(LOG_CFG, "Unsupported automat config version " << data.version);
		return MemoryResult_WrongVersion;
	}

	setAutomatId(data.automatId);
	setConfigId(data.configId);
	setPaymentBus(data.paymentBus);
	setTaxSystem(data.taxSystem);
	setDecimalPoint(data.decimalPoint);
	return MemoryResult_Ok;
}

MemoryResult Config1Automat::saveData(Memory *memory) {
	Config1AutomatStruct data;
	data.version = CONFIG_AUTOMAT_VERSION1;
	data.automatId = automatId;
	data.configId = configId;
	data.paymentBus = paymentBus;
	data.taxSystem = taxSystem;
	data.decimalPoint = decimalPoint;
	data.devider = devider;

	MemoryCrc crc(memory);
	crc.startCrc();
	crc.write(&data, sizeof(data));
	crc.writeCrc();
	return MemoryResult_Ok;
}

void Config1Automat::setAutomatId(uint32_t automatId) {
	this->automatId = automatId;
}

void Config1Automat::setConfigId(uint32_t configId) {
	this->configId = configId;
}

void Config1Automat::setPaymentBus(uint16_t paymentBus) {
	if(paymentBus > PaymentBus_MaxValue) {
		LOG_ERROR(LOG_CFG, "Wrong payment bus value " << paymentBus);
		return;
	}
	this->paymentBus = paymentBus;
}

uint16_t Config1Automat::getPaymentBus() const {
	return paymentBus;
}

void Config1Automat::setTaxSystem(uint16_t taxSystem) {
	this->taxSystem = taxSystem;
}

uint16_t Config1Automat::getTaxSystem() const {
	return taxSystem;
}

void Config1Automat::setDecimalPoint(uint16_t decimalPoint) {
	if(decimalPoint > DECIMAL_POINT_MAX) {
		LOG_ERROR(LOG_CFG, "Wrong decimal point value " << paymentBus);
		return;
	}
	this->decimalPoint = decimalPoint;
	this->devider = calcDevider(decimalPoint);
}

uint32_t Config1Automat::calcDevider(uint16_t decimalPoint) {
	uint32_t d = 1;
	for(uint16_t i = 0; i < decimalPoint; i++) {
		d = d * 10;
	}
	return d;
}

uint32_t Config1Automat::convertValueToMoney(uint32_t value) const {
	return (value / devider);
}

uint32_t Config1Automat::convertMoneyToValue(uint32_t money) const {
	return (money * devider);
}

bool Config1Automat::addProduct(const char *selectId) {
	return productIndexes.add(selectId);
}

bool Config1Automat::addProduct(const char *selectId, uint16_t cashlessId) {
	return productIndexes.add(selectId, cashlessId);
}

void Config1Automat::addPriceList(const char *device, uint8_t number) {
	priceIndexes.add(device, number);
}
