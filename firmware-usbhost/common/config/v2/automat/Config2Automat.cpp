#include "Config2Automat.h"
#include "Config2AutomatData.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "mdb/MdbProtocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Number.h"
#include "utils/include/DecimalPoint.h"
#include "logger/include/Logger.h"

#include <string.h>

Config2Automat::Config2Automat() {
	setDefault();
}

void Config2Automat::setDefault() {
	setConfigId(0);
	setAutomatId(0);
	setPaymentBus(Config2Automat::PaymentBus_MdbMaster);
	setDecimalPoint(DECIMAL_POINT_DEFAULT);
	setTaxSystem(Fiscal::TaxSystem_ENVD);
	setCurrency(RUSSIAN_CURRENCY_RUB);
	setMaxCredit(MAX_CREDIT_DEFAULT);
	changeWithoutSale = false;
	lockSaleByFiscal = true;
	freeSale = false;
	lotteryPeriod = 0;
}

void Config2Automat::init() {
	for(uint16_t i = 0; i < productIndexes.getSize(); i++) {
		Config2Product *product = new Config2Product;
		product->setDefault();
		for(uint16_t j = 0; j < priceIndexes.getSize(); j++) {
			Config2Price *price = new Config2Price;
			price->setDefault();
			product->prices.add(price);
		}
		products.add(product);
	}
	products.set(priceIndexes.getSize(), productIndexes.getSize());
}

MemoryResult Config2Automat::load(Memory *memory) {
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

	LOG_INFO(LOG_CFG, "Automat config OK");
	return MemoryResult_Ok;
}

MemoryResult Config2Automat::save(Memory *memory) {
	MemoryResult result = saveData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat save failed");
		return result;
	}
	result = products.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Products save failed");
		return result;
	}
	result = productIndexes.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Product indexes save failed");
		return result;
	}
	result = priceIndexes.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Price indexes save failed");
		return result;
	}

	LOG_INFO(LOG_CFG, "Automat config saved");
	return MemoryResult_Ok;
}

MemoryResult Config2Automat::loadData(Memory *memory) {
	Config3AutomatStruct data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong automat config CRC");
		return result;
	}
	if(data.version != CONFIG_AUTOMAT_VERSION2) {
		LOG_ERROR(LOG_CFG, "Wrong automat config version " << data.version);
		return MemoryResult_WrongVersion;
	}

	setAutomatId(data.automatId);
	setConfigId(data.configId);
	setPaymentBus(data.paymentBus);
	setTaxSystem(data.taxSystem);
	setCurrency(data.currency);
	setDecimalPoint(data.decimalPoint);
	setMaxCredit(data.maxCredit);
	return MemoryResult_Ok;
}

MemoryResult Config2Automat::saveData(Memory *memory) {
	Config2AutomatStruct data;
	data.version = CONFIG_AUTOMAT_VERSION2;
	data.automatId = automatId;
	data.configId = configId;
	data.paymentBus = paymentBus;
	data.taxSystem = taxSystem;
	data.currency = currency;
	data.decimalPoint = decimalPoint;
	data.maxCredit = maxCredit;
	data.changeWithoutSale = false;
	data.lockSaleByFiscal = true;
	data.freeSale = false;
	data.workWeek = 0x7F;
	data.workHour = 0;
	data.workMinute = 0;
	data.workSecond = 0;
	data.workInterval = 86400;
	data.lotteryPeriod = 0;
	data.lotteryWeek = 0;
	data.lotteryHour = 0;
	data.lotteryMinute = 0;
	data.lotterySecond = 0;
	data.lotteryInterval = 0;

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config2Automat::setAutomatId(uint32_t automatId) {
	this->automatId = automatId;
}

void Config2Automat::setConfigId(uint32_t configId) {
	this->configId = configId;
}

void Config2Automat::setPaymentBus(uint16_t paymentBus) {
	if(paymentBus > Config2Automat::PaymentBus_MaxValue) {
		LOG_ERROR(LOG_CFG, "Wrong payment bus value " << paymentBus);
		return;
	}
	this->paymentBus = paymentBus;
}

uint16_t Config2Automat::getPaymentBus() const {
	return paymentBus;
}

void Config2Automat::setTaxSystem(uint16_t taxSystem) {
	this->taxSystem = taxSystem;
}

uint16_t Config2Automat::getTaxSystem() const {
	return taxSystem;
}

void Config2Automat::setCurrency(uint16_t currency) {
	if(currency != RUSSIAN_CURRENCY_RUB && currency != RUSSIAN_CURRENCY_RUR) {
		LOG_ERROR(LOG_CFG, "Wrong currency value " << currency);
		this->currency = RUSSIAN_CURRENCY_RUB;
		return;
	}
	this->currency = currency;
}

uint16_t Config2Automat::getCurrency() const {
	return currency;
}

void Config2Automat::setDecimalPoint(uint16_t decimalPoint) {
	if(decimalPoint > DECIMAL_POINT_MAX) {
		LOG_ERROR(LOG_CFG, "Wrong decimal point value " << paymentBus);
		return;
	}
	this->decimalPoint = decimalPoint;
	this->devider = calcDevider(decimalPoint);
}

uint32_t Config2Automat::calcDevider(uint16_t decimalPoint) {
	uint32_t d = 1;
	for(uint16_t i = 0; i < decimalPoint; i++) {
		d = d * 10;
	}
	return d;
}

uint32_t Config2Automat::convertValueToMoney(uint32_t value) const {
	return (value / devider);
}

uint32_t Config2Automat::convertMoneyToValue(uint32_t money) const {
	return (money * devider);
}

void Config2Automat::setMaxCredit(uint32_t maxCredit) {
	this->maxCredit = maxCredit;
}

uint32_t Config2Automat::getMaxCredit() const {
	return maxCredit;
}

bool Config2Automat::addProduct(const char *selectId, uint16_t cashlessId) {
	return productIndexes.add(selectId, cashlessId);
}

void Config2Automat::addPriceList(const char *device, uint8_t number, Config3PriceIndexType type) {
	priceIndexes.add(device, number, type);
}
