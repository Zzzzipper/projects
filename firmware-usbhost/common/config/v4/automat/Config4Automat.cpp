#include "Config4Automat.h"
#include "Config4AutomatData.h"
#include "evadts/EvadtsProtocol.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "mdb/MdbProtocol.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Number.h"
#include "utils/include/DecimalPoint.h"
#include "logger/include/Logger.h"

#include <string.h>

#define MAX_CREDIT_DEFAULT 50000

Config4Automat::Config4Automat(RealTimeInterface *realtime) :
	data(realtime),
	devices(DECIMAL_POINT_DEFAULT, realtime, MAX_CREDIT_DEFAULT)
{
}

void Config4Automat::shutdown() {
	LOG_DEBUG(LOG_CFG, "shutdown");
	setConfigId(0);
	productIndexes.clear();
	priceIndexes.clear();
}

MemoryResult Config4Automat::init(Memory *memory) {
	if(priceIndexes.getSize() <= 0) {
		priceIndexes.add("CA", 0, Config3PriceIndexType_Base);
	}

	MemoryResult result = data.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = paymentStat.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = devices.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = products.init(productIndexes.getSize(), priceIndexes.getSize(), memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = productIndexes.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = priceIndexes.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

void Config4Automat::asyncInitStart(Memory *memory) {
	if(priceIndexes.getSize() <= 0) {
		priceIndexes.add("CA", 0, Config3PriceIndexType_Base);
	}

	data.init(memory);
	paymentStat.init(memory);
	devices.init(memory);
	products.asyncInitStart(productIndexes.getSize(), priceIndexes.getSize(), memory);
}

bool Config4Automat::asyncInitProc(Memory *memory) {
	if(products.asyncInitProc(memory) == true) {
		return true;
	}

	productIndexes.init(memory);
	priceIndexes.init(memory);
	return false;
}

MemoryResult Config4Automat::load(Memory *memory) {
	MemoryResult result = data.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat load failed");
		return result;
	}
	result = paymentStat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "PaymentStat load failed");
		return result;
	}
	result = devices.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Devices load failed");
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

MemoryResult Config4Automat::loadAndCheck(Memory *memory) {
	MemoryResult result = data.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat load failed");
		return result;
	}
	result = paymentStat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "PaymentStat load failed");
		return result;
	}
	result = devices.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Devices load failed");
		return result;
	}
	result = products.loadAndCheck(memory);
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

//todo: разобраться с этим методом очень похоже, что что-то сломано
MemoryResult Config4Automat::reinit(Memory *memory) {
	uint32_t start = memory->getAddress();
	MemoryResult result = data.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat load failed");
		memory->setAddress(start);
		return init(memory);
	}
	result = paymentStat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Devices load failed");
		memory->setAddress(start);
		return init(memory);
	}
	result = devices.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Devices load failed");
		memory->setAddress(start);
		return init(memory);
	}
	result = products.loadAndCheck(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Products load failed");
		memory->setAddress(start);
		return init(memory);
	}
	result = productIndexes.load(products.getProductNum(), memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Product indexes load failed");
		memory->setAddress(start);
		return init(memory);
	}
	result = priceIndexes.load(products.getPriceListNum(), memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Price indexes load failed");
		memory->setAddress(start);
		return init(memory);
	}
	return MemoryResult_Ok;
}

MemoryResult Config4Automat::save() {
	MemoryResult result = data.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = products.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = priceIndexes.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config4Automat::reset() {
	LOG_DEBUG(LOG_CFG, "reset");
	MemoryResult result = paymentStat.reset();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = products.reset();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

void Config4Automat::setAutomatId(uint32_t automatId) { data.setAutomatId(automatId); }
uint32_t Config4Automat::getAutomatId() const { return data.getAutomatId(); }
void Config4Automat::setConfigId(uint32_t configId) { data.setConfigId(configId); }
uint32_t Config4Automat::getConfigId() const { return data.getConfigId(); }
void Config4Automat::setPaymentBus(uint16_t paymentBus) { data.setPaymentBus(paymentBus); }
uint16_t Config4Automat::getPaymentBus() const { return data.getPaymentBus(); }
void Config4Automat::setEvadts(bool evadts) { data.setEvadts(evadts); }
bool Config4Automat::getEvadts() const { return data.getEvadts(); }
void Config4Automat::setTaxSystem(uint16_t taxSystem) { data.setTaxSystem(taxSystem); }
uint16_t Config4Automat::getTaxSystem() const { return data.getTaxSystem(); }
void Config4Automat::setCurrency(uint16_t currency) { data.setCurrency(currency); }
uint16_t Config4Automat::getCurrency() const { return data.getCurrency(); }
void Config4Automat::setDecimalPoint(uint16_t decimalPoint) {
	data.setDecimalPoint(decimalPoint);
	devices.setMasterDecimalPoint(decimalPoint);
}
uint16_t Config4Automat::getDecimalPoint() const { return data.getDecimalPoint(); }
void Config4Automat::setMaxCredit(uint32_t maxCredit) {
	data.setMaxCredit(maxCredit);
	devices.getBVContext()->setMaxBill(maxCredit);
}
uint32_t Config4Automat::getMaxCredit() const { return data.getMaxCredit(); }
void Config4Automat::setExt1Device(uint8_t ext1Device) { data.setExt1Device(ext1Device); }
uint8_t Config4Automat::getExt1Device() { return data.getExt1Device(); }
void Config4Automat::setCashlessNumber(uint8_t cashlessNum) { data.setCashlessNumber(cashlessNum); }
uint8_t Config4Automat::getCashlessNumber() { return data.getCashlessNumber(); }
void Config4Automat::setCashlessMaxLevel(uint8_t cashlessMaxLevel) { data.setCashlessMaxLevel(cashlessMaxLevel); }
uint8_t Config4Automat::getCashlessMaxLevel() { return data.getCashlessMaxLevel(); }
void Config4Automat::setScaleFactor(uint32_t scaleFactor) { data.setScaleFactor(scaleFactor); }
uint8_t Config4Automat::getScaleFactor() { return data.getScaleFactor(); }
void Config4Automat::setQrCode(uint32_t qrCode) { data.setQrCode(qrCode); }
uint8_t Config4Automat::getQrCode() { return data.getQrCode(); }
void Config4Automat::setPriceHolding(bool priceHolding) { data.setPriceHolding(priceHolding); }
bool Config4Automat::getPriceHolding() { return data.getPriceHolding(); }
void Config4Automat::setCategoryMoney(bool categoryMoney) { data.setCategoryMoney(categoryMoney); }
bool Config4Automat::getCategoryMoney() { return data.getCategoryMoney(); }
void Config4Automat::setShowChange(bool showChange) { data.setShowChange(showChange); }
bool Config4Automat::getShowChange() { return data.getShowChange(); }
void Config4Automat::setCreditHolding(bool creditHolding) { data.setCreditHolding(creditHolding); }
bool Config4Automat::getCreditHolding() { return data.getCreditHolding(); }
void Config4Automat::setMultiVend(bool multiVend) { data.setMultiVend(multiVend); }
bool Config4Automat::getMultiVend() { return data.getMultiVend(); }
void Config4Automat::setCashless2Click(bool cashless2Click) { data.setCashless2Click(cashless2Click); }
bool Config4Automat::getCashless2Click() { return data.getCashless2Click(); }

Mdb::DeviceContext *Config4Automat::getSMContext() { return data.getContext(); }
MdbCoinChangerContext *Config4Automat::getCCContext() { return devices.getCCContext(); }
MdbBillValidatorContext *Config4Automat::getBVContext() { return devices.getBVContext(); }
Mdb::DeviceContext *Config4Automat::getMdb1CashlessContext() { return devices.getCL1Context(); }
Mdb::DeviceContext *Config4Automat::getMdb2CashlessContext() { return devices.getCL2Context(); }
Mdb::DeviceContext *Config4Automat::getExt1CashlessContext() { return devices.getCL3Context(); }
Mdb::DeviceContext *Config4Automat::getUsb1CashlessContext() { return devices.getCL4Context(); }
Fiscal::Context *Config4Automat::getFiscalContext() { return devices.getFiscalContext(); }

const char *Config4Automat::getCommunictionId() {
	return "EPHOR00001";
}

Config4DeviceList *Config4Automat::getDeviceList() {
	return &devices;
}

bool Config4Automat::addProduct(const char *selectId, uint16_t cashlessId) {
	return productIndexes.add(selectId, cashlessId);
}

void Config4Automat::addPriceList(const char *device, uint8_t number, Config3PriceIndexType type) {
	priceIndexes.add(device, number, type);
}

Config4ProductIterator *Config4Automat::createIterator() {
	return new Config4ProductIterator(&priceIndexes, &productIndexes, &products);
}

MemoryResult Config4Automat::sale(const char *selectId, const char *device, uint16_t number, uint32_t value) {
	uint16_t productIndex = productIndexes.getIndex(selectId);
	if(productIndex >= productIndexes.getSize()) {
		LOG_ERROR(LOG_CFG, "Product " << selectId << " not exist");
		return MemoryResult_NotFound;
	}
	uint16_t priceIndex = priceIndexes.getIndex(device, number);
	if(priceIndex >= priceIndexes.getSize()) {
		LOG_ERROR(LOG_CFG, "Price list " << device << number << " not exist");
		return MemoryResult_NotFound;
	}
	return products.sale(device, productIndex, priceIndex, value);
}

MemoryResult Config4Automat::sale(uint16_t cashlessId, const char *device, uint16_t number, uint32_t value) {
	uint16_t productIndex = productIndexes.getIndex(cashlessId);
	if(productIndex >= productIndexes.getSize()) {
		LOG_ERROR(LOG_CFG, "Product " << cashlessId << " not exist");
		return MemoryResult_NotFound;
	}
	uint16_t priceIndex = priceIndexes.getIndex(device, number);
	if(priceIndex >= priceIndexes.getSize()) {
		LOG_ERROR(LOG_CFG, "Price list " << device << number << " not exist");
		return MemoryResult_NotFound;
	}
	return products.sale(device, productIndex, priceIndex, value);
}

MemoryResult Config4Automat::registerCoinIn(uint32_t value, uint8_t route) {
	return paymentStat.registerCoinIn(value, route);
}

MemoryResult Config4Automat::registerCoinDispense(uint32_t value) {
	return paymentStat.registerCoinDispense(value);
}

MemoryResult Config4Automat::registerCoinDispenseManually(uint32_t value) {
	return paymentStat.registerCoinDispenseManually(value);
}

MemoryResult Config4Automat::registerCoinFill(uint32_t value) {
	return paymentStat.registerCoinFill(value);
}

MemoryResult Config4Automat::registerBillIn(uint32_t value) {
	return paymentStat.registerBillIn(value);
}

MemoryResult Config4Automat::registerCashOverpay(uint32_t value) {
	return paymentStat.registerCashOverpay(value);
}
