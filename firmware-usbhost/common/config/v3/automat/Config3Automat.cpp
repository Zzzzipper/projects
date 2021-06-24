#include "Config3Automat.h"
#include "Config3AutomatData.h"
#include "evadts/EvadtsProtocol.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "mdb/MdbProtocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Number.h"
#include "utils/include/DecimalPoint.h"
#include "logger/include/Logger.h"

#include <string.h>

#define MAX_CREDIT_DEFAULT 50000
#define CASHLESS1_NUM 1
#define CASHLESS2_NUM 2
#define SCALE_FACTOR_MIN 1
#define SCALE_FACTOR_MAX 255

Config3Automat::Config3Automat(RealTimeInterface *realtime) :
	smContext(DECIMAL_POINT_DEFAULT, realtime),
	cl3Context(DECIMAL_POINT_DEFAULT, realtime),
	cl4Context(DECIMAL_POINT_DEFAULT, realtime),
	remoteCashlessContext(DECIMAL_POINT_DEFAULT, realtime),
	fiscalContext(DECIMAL_POINT_DEFAULT, realtime),
	screenContext(DECIMAL_POINT_DEFAULT, realtime),
	nfcContext(DECIMAL_POINT_DEFAULT, realtime),
	devices(DECIMAL_POINT_DEFAULT, realtime, MAX_CREDIT_DEFAULT)
{
	setDefault();
}

void Config3Automat::setDefault() {
	flags = 0;
	setAutomatId(0);
	setConfigId(0);
	setTaxSystem(Fiscal::TaxSystem_ENVD);

	setInternetDevice(InternetDevice_Gsm);
	setExt1Device(0);
	setExt2Device(0);
	setUsb1Device(0);
	setQrType(0);
	setFiscalPrinter(0);
	settings.setDefault();

	setPaymentBus(PaymentBus_MdbMaster);
	setEvadts(false);
	setCurrency(RUSSIAN_CURRENCY_RUB);
	setDecimalPoint(DECIMAL_POINT_DEFAULT);
	setMaxCredit(MAX_CREDIT_DEFAULT);
	setCashlessNumber(2);
	setCashlessMaxLevel(Mdb::FeatureLevel_1);
	setScaleFactor(1);
	setCategoryMoney(false);
	setShowChange(true);
	setPriceHolding(false);
	setCreditHolding(false);
	setMultiVend(false);
	setCashless2Click(false);

	devices.getBVContext()->setUseChange(true);
}

void Config3Automat::shutdown() {
	LOG_DEBUG(LOG_CFG, "shutdown");
	setConfigId(0);
	productIndexes.clear();
	priceIndexes.clear();
}

MemoryResult Config3Automat::init(Memory *memory) {
	if(priceIndexes.getSize() <= 0) {
		priceIndexes.add("CA", 0, Config3PriceIndexType_Base);
	}

	MemoryResult result = initData(memory);
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
	result = settings.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
#ifdef TA_PALMA
	result = devices.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
#endif
	result = barcodes.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = wareGroups.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = wareLinks.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

void Config3Automat::asyncInitStart(Memory *memory) {
	if(priceIndexes.getSize() <= 0) {
		priceIndexes.add("CA", 0, Config3PriceIndexType_Base);
	}

	initData(memory);
	products.asyncInitStart(productIndexes.getSize(), priceIndexes.getSize(), memory);
}

bool Config3Automat::asyncInitProc() {
	if(products.asyncInitProc() == true) {
		return true;
	}

	productIndexes.init(memory);
	priceIndexes.init(memory);
	settings.init(memory);
#ifdef TA_PALMA
	devices.init(memory);
#endif
	barcodes.init(memory);
	wareGroups.init(memory);
	wareLinks.init(memory);
	return false;
}

MemoryResult Config3Automat::load(Memory *memory) {
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
	uint32_t settingsAddress = memory->getAddress();
	result = settings.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Settings load failed");
		memory->setAddress(settingsAddress);
		result = settings.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Settings init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Settings init OK");
	}
#ifdef TA_PALMA
	uint32_t devicesAddress = memory->getAddress();
	result = devices.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Devices load failed");
		memory->setAddress(devicesAddress);
		result = devices.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Devices init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Devices init OK");
	}
#endif
	result = barcodes.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Barcodes load failed");
		return result;
	}
	result = wareGroups.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "WareGroups load failed");
		return result;
	}
	result = wareLinks.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "WareLinks load failed");
		return result;
	}
	LOG_INFO(LOG_CFG, "Automat config OK");
	return MemoryResult_Ok;
}

MemoryResult Config3Automat::loadAndCheck(Memory *memory) {
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat load failed");
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
	uint32_t settingsAddress = memory->getAddress();
	result = settings.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Settings load failed");
		memory->setAddress(settingsAddress);
		result = settings.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Settings init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Settings init OK");
	}
#ifdef TA_PALMA
	uint32_t devicesAddress = memory->getAddress();
	result = devices.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Devices load failed");
		memory->setAddress(devicesAddress);
		result = devices.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Devices init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Devices init OK");
	}
#endif
	result = barcodes.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Barcodes load failed");
		return result;
	}
	result = wareGroups.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "WareGroups load failed");
		return result;
	}
	result = wareLinks.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "WareLinks load failed");
		return result;
	}
	LOG_INFO(LOG_CFG, "Automat config OK");
	return MemoryResult_Ok;
}

MemoryResult Config3Automat::reinit(Memory *memory) {
	uint32_t start = memory->getAddress();
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat load failed");
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
	uint32_t settingsAddress = memory->getAddress();
	result = settings.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Settings load failed");
		memory->setAddress(settingsAddress);
		result = settings.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Settings init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Settings init OK");
	}
#ifdef TA_PALMA
	uint32_t devicesAddress = memory->getAddress();
	result = devices.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Devices load failed");
		memory->setAddress(devicesAddress);
		result = devices.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Devices init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Devices init OK");
	}
#endif
	settingsAddress = memory->getAddress();
	result = barcodes.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Barcodes load failed");
		memory->setAddress(settingsAddress);
		result = barcodes.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Barcodes init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Settings init OK");
	}
	settingsAddress = memory->getAddress();
	result = wareGroups.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "WareGroups load failed");
		memory->setAddress(settingsAddress);
		result = wareGroups.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "WareGroups init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Settings init OK");
	}
	settingsAddress = memory->getAddress();
	result = wareLinks.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "WareLinks load failed");
		memory->setAddress(settingsAddress);
		result = wareLinks.init(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "WareLinks init failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Settings init OK");
	}
	return MemoryResult_Ok;
}

MemoryResult Config3Automat::save() {
	MemoryResult result = saveData();
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
	result = settings.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = barcodes.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = wareGroups.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = wareLinks.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3Automat::reset() {
	LOG_DEBUG(LOG_CFG, "reset");
	return products.reset();
}

/**
 * TODO: Костыль для сохранения значения двоичной точки при инициализации.
 * Видимо метафора кривая и значение двоичной точки должно принадлежать другой сущности.
 */
MemoryResult Config3Automat::initData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	uint16_t decimalPoint = smContext.getDecimalPoint();
	setDefault();
	setDecimalPoint(decimalPoint);
	return saveData();
}

MemoryResult Config3Automat::loadData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	Config3AutomatStruct data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong automat config CRC");
		return result;
	}
	if(data.version != CONFIG3_AUTOMAT_VERSION) {
		LOG_ERROR(LOG_CFG, "Wrong automat config version " << data.version);
		return MemoryResult_WrongVersion;
	}

	setAutomatId(data.automatId);
	setConfigId(data.configId);
	setTaxSystem(data.taxSystem);

	setInternetDevice(data.internetDevice);
	setExt1Device(data.ext1Device);
	setExt2Device(data.ext2Device);
	setUsb1Device(data.usb1Device);
	setQrType(data.qrType);
	setFiscalPrinter(data.fiscalPrinter);

	setPaymentBus(data.paymentBus);
	setCurrency(data.currency);
	setDecimalPoint(data.decimalPoint);
	setMaxCredit(data.maxCredit);
	this->flags = data.flags;
	setCashlessNumber(data.cashlessNum);
	setCashlessMaxLevel(data.cashlessMaxLevel);
	setScaleFactor(data.scaleFactor);

	devices.getBVContext()->setUseChange(data.bvOn);
	return MemoryResult_Ok;
}

MemoryResult Config3Automat::saveData() {
	memory->setAddress(address);
	Config3AutomatStruct data;
	data.version = CONFIG3_AUTOMAT_VERSION;
	data.automatId = automatId;
	data.configId = configId;
	data.paymentBus = paymentBus;
	data.taxSystem = taxSystem;
	data.currency = smContext.getCurrency();
	data.decimalPoint = smContext.getDecimalPoint();
	data.maxCredit = maxCredit;
	data.flags = flags;
	data.ext1Device = ext1Device;
	data.freeSale = 0;
	data.cashlessNum = cashlessNum;
	data.scaleFactor = smContext.getScalingFactor();
	data.cashlessMaxLevel = cashlessMaxLevel;
	data.fiscalPrinter = fiscalPrinter;
	data.internetDevice = internetDevice;
	data.ext2Device = ext2Device;
	data.usb1Device = usb1Device;
	data.qrType = qrType;
	data.bvOn = devices.getBVContext()->getUseChange();
	memset(data.reserved, 0, sizeof(data.reserved));

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config3Automat::setAutomatId(uint32_t automatId) {
	this->automatId = automatId;
}

void Config3Automat::setConfigId(uint32_t configId) {
	this->configId = configId;
}

void Config3Automat::setTaxSystem(uint16_t taxSystem) {
	this->taxSystem = taxSystem;
}

uint16_t Config3Automat::getTaxSystem() const {
	return taxSystem;
}

void Config3Automat::setInternetDevice(uint8_t internetDevice) {
	if(internetDevice == 0) {
		this->internetDevice = InternetDevice_Gsm;
		return;
	}
	this->internetDevice = internetDevice;
}

uint8_t Config3Automat::getInternetDevice() {
	return internetDevice;
}

void Config3Automat::setExt1Device(uint8_t ext1Device) {
	this->ext1Device = ext1Device;
}

uint8_t Config3Automat::getExt1Device() {
	return ext1Device;
}

void Config3Automat::setExt2Device(uint8_t ext2Device) {
	this->ext2Device = ext2Device;
}

uint8_t Config3Automat::getExt2Device() {
	return ext2Device;
}

void Config3Automat::setUsb1Device(uint8_t usb1Device) {
	this->usb1Device = usb1Device;
}

uint8_t Config3Automat::getUsb1Device() {
	return usb1Device;
}

void Config3Automat::setQrType(uint32_t qrType) {
	this->qrType = qrType;
}

uint8_t Config3Automat::getQrType() {
	return qrType;
}

void Config3Automat::setFiscalPrinter(uint32_t fiscalPrinter) {
	if(fiscalPrinter > 3) {
		return;
	}
	this->fiscalPrinter = fiscalPrinter;
}

uint8_t Config3Automat::getFiscalPrinter() {
	return fiscalPrinter;
}

void Config3Automat::setEthMac(uint8_t *mac) {
	settings.setEthMac(mac);
}

void Config3Automat::setEthMac(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5) {
	settings.setEthMac(mac0, mac1, mac2, mac3, mac4, mac5);
}

uint8_t *Config3Automat::getEthMac() {
	return settings.getEthMac();
}

uint16_t Config3Automat::getEthMacLen() {
	return settings.getEthMacLen();
}

void Config3Automat::setEthAddr(uint32_t addr) {
	settings.setEthAddr(addr);
}

uint32_t Config3Automat::getEthAddr() {
	return settings.getEthAddr();
}

void Config3Automat::setEthMask(uint32_t addr) {
	settings.setEthMask(addr);
}

uint32_t Config3Automat::getEthMask() {
	return settings.getEthMask();
}

void Config3Automat::setEthGateway(uint32_t addr) {
	settings.setEthGateway(addr);
}

uint32_t Config3Automat::getEthGateway() {
	return settings.getEthGateway();
}

void Config3Automat::setFidType(uint16_t type) {
	settings.setFidType(type);
}

uint16_t Config3Automat::getFidType() {
	return settings.getFidType();
}

void Config3Automat::setFidDevice(const char *device) {
	settings.setFidDevice(device);
}

const char *Config3Automat::getFidDevice() {
	return settings.getFidDevice();
}

void Config3Automat::setFidAddr(const char *addr) {
	settings.setFidAddr(addr);
}

const char *Config3Automat::getFidAddr() {
	return settings.getFidAddr();
}

void Config3Automat::setFidPort(uint16_t port) {
	settings.setFidPort(port);
}

uint16_t Config3Automat::getFidPort() {
	return settings.getFidPort();
}

void Config3Automat::setFidUsername(const char *username) {
	settings.setFidUsername(username);
}

const char *Config3Automat::getFidUsername() {
	return settings.getFidUsername();
}

void Config3Automat::setFidPassword(const char *password) {
	settings.setFidPassword(password);
}

const char *Config3Automat::getFidPassword() {
	return settings.getFidPassword();
}

void Config3Automat::setPaymentBus(uint16_t paymentBus) {
	if(paymentBus > PaymentBus_MaxValue) {
		LOG_ERROR(LOG_CFG, "Wrong payment bus value " << paymentBus);
		return;
	}
	this->paymentBus = paymentBus;
}

uint16_t Config3Automat::getPaymentBus() const {
	return paymentBus;
}

void Config3Automat::setEvadts(bool evadts) {
	if(evadts > 0) {
		this->flags |= ConfigAutomatFlag_Evadts;
	} else {
		this->flags &= ~ConfigAutomatFlag_Evadts;
	}
}

bool Config3Automat::getEvadts() const {
	return (flags & ConfigAutomatFlag_Evadts) > 0;
}

void Config3Automat::setCurrency(uint16_t currency) {
	if(currency != RUSSIAN_CURRENCY_RUB && currency != RUSSIAN_CURRENCY_RUR) {
		LOG_ERROR(LOG_CFG, "Wrong currency value " << currency);
		currency = RUSSIAN_CURRENCY_RUB;
	}
	this->smContext.setCurrency(currency);
}

uint16_t Config3Automat::getCurrency() const {
	return smContext.getCurrency();
}

void Config3Automat::setDecimalPoint(uint16_t decimalPoint) {
	if(decimalPoint > DECIMAL_POINT_MAX) {
		LOG_ERROR(LOG_CFG, "Wrong decimal point value " << paymentBus);
		return;
	}
	this->smContext.init(decimalPoint, smContext.getScalingFactor());
	this->smContext.setMasterDecimalPoint(decimalPoint);
	this->cl3Context.setMasterDecimalPoint(decimalPoint);
	this->cl4Context.setMasterDecimalPoint(decimalPoint);
	this->remoteCashlessContext.setMasterDecimalPoint(decimalPoint);
	this->fiscalContext.setMasterDecimalPoint(decimalPoint);
	this->screenContext.setMasterDecimalPoint(decimalPoint);
	this->nfcContext.setMasterDecimalPoint(decimalPoint);
	this->devices.setMasterDecimalPoint(decimalPoint);
}

uint16_t Config3Automat::getDecimalPoint() const {
	return smContext.getDecimalPoint();
}

void Config3Automat::setMaxCredit(uint32_t maxCredit) {
	this->maxCredit = maxCredit;
	this->devices.getBVContext()->setMaxBill(maxCredit);
}

uint32_t Config3Automat::getMaxCredit() const {
	return maxCredit;
}

void Config3Automat::setCashlessNumber(uint8_t cashlessNum) {
	if(cashlessNum != CASHLESS1_NUM && cashlessNum != CASHLESS2_NUM) {
		return;
	}
	this->cashlessNum = cashlessNum;
}

uint8_t Config3Automat::getCashlessNumber() {
	return cashlessNum;
}

void Config3Automat::setCashlessMaxLevel(uint8_t cashlessMaxLevel) {
	if(cashlessMaxLevel < Mdb::FeatureLevel_1 || cashlessMaxLevel > Mdb::FeatureLevel_3) {
		return;
	}
	this->cashlessMaxLevel = cashlessMaxLevel;
}

uint8_t Config3Automat::getCashlessMaxLevel() {
	return cashlessMaxLevel;
}

void Config3Automat::setScaleFactor(uint32_t scaleFactor) {
	if(scaleFactor < SCALE_FACTOR_MIN || scaleFactor > SCALE_FACTOR_MAX) {
		return;
	}
	smContext.init(smContext.getDecimalPoint(), scaleFactor);
}

uint8_t Config3Automat::getScaleFactor() {
	return smContext.getScalingFactor();
}

void Config3Automat::setPriceHolding(bool priceHolding) {
	if(priceHolding > 0) {
		this->flags |= ConfigAutomatFlag_PriceHolding;
	} else {
		this->flags &= ~ConfigAutomatFlag_PriceHolding;
	}
}

bool Config3Automat::getPriceHolding() {
	return (flags & ConfigAutomatFlag_PriceHolding) > 0;
}

void Config3Automat::setCategoryMoney(bool categoryMoney) {
	if(categoryMoney > 0) {
		this->flags |= ConfigAutomatFlag_CategoryMoney;
	} else {
		this->flags &= ~ConfigAutomatFlag_CategoryMoney;
	}
}

bool Config3Automat::getCategoryMoney() {
	return (flags & ConfigAutomatFlag_CategoryMoney) > 0;
}

void Config3Automat::setShowChange(bool showChange) {
	if(showChange > 0) {
		this->flags |= ConfigAutomatFlag_ShowChange;
	} else {
		this->flags &= ~ConfigAutomatFlag_ShowChange;
	}
}

bool Config3Automat::getShowChange() {
	return (flags & ConfigAutomatFlag_ShowChange) > 0;
}

void Config3Automat::setCreditHolding(bool creditHolding) {
	if(creditHolding > 0) {
		this->flags |= ConfigAutomatFlag_CreditHolding;
	} else {
		this->flags &= ~ConfigAutomatFlag_CreditHolding;
	}
}

bool Config3Automat::getCreditHolding() {
	return (flags & ConfigAutomatFlag_CreditHolding) > 0;
}

void Config3Automat::setMultiVend(bool multiVend) {
	if(multiVend > 0) {
		this->flags |= ConfigAutomatFlag_MultiVend;
	} else {
		this->flags &= ~ConfigAutomatFlag_MultiVend;
	}
}

bool Config3Automat::getMultiVend() {
	return (flags & ConfigAutomatFlag_MultiVend) > 0;
}

void Config3Automat::setCashless2Click(bool cashless2Click) {
	if(cashless2Click > 0) {
		this->flags |= ConfigAutomatFlag_Cashless2Click;
	} else {
		this->flags &= ~ConfigAutomatFlag_Cashless2Click;
	}
}

bool Config3Automat::getCashless2Click() {
	return (flags & ConfigAutomatFlag_Cashless2Click) > 0;
}

Mdb::DeviceContext *Config3Automat::getSMContext() {
	return &smContext;
}

MdbCoinChangerContext *Config3Automat::getCCContext() {
	return devices.getCCContext();
}

MdbBillValidatorContext *Config3Automat::getBVContext() {
	return devices.getBVContext();
}

Mdb::DeviceContext *Config3Automat::getMdb1CashlessContext() {
	return devices.getCL1Context();
}

Mdb::DeviceContext *Config3Automat::getMdb2CashlessContext() {
	return devices.getCL2Context();
}

Mdb::DeviceContext *Config3Automat::getExt1CashlessContext() {
	return &cl3Context;
}

Mdb::DeviceContext *Config3Automat::getUsb1CashlessContext() {
	return &cl4Context;
}

Mdb::DeviceContext *Config3Automat::getRemoteCashlessContext() {
	return &remoteCashlessContext;
}

Fiscal::Context *Config3Automat::getFiscalContext() {
	return &fiscalContext;
}

Mdb::DeviceContext *Config3Automat::getScreenContext() {
	return &screenContext;
}

Mdb::DeviceContext *Config3Automat::getNfcContext() {
	return &nfcContext;
}

const char *Config3Automat::getCommunictionId() {
	return "EPHOR00001";
}

Config3DeviceList *Config3Automat::getDeviceList() {
	return &devices;
}

bool Config3Automat::addProduct(const char *selectId, uint16_t cashlessId) {
	return productIndexes.add(selectId, cashlessId);
}

void Config3Automat::addPriceList(const char *device, uint8_t number, Config3PriceIndexType type) {
	priceIndexes.add(device, number, type);
}

Config3ProductIterator *Config3Automat::createIterator() {
	return new Config3ProductIterator(&priceIndexes, &productIndexes, &products);
}

MemoryResult Config3Automat::sale(Fiscal::Sale *saleData) {
	for(uint16_t i = 0; i < saleData->getProductNum(); i++) {
		Fiscal::Product *product = saleData->getProduct(i);
		for(uint16_t q = 0; q < product->quantity; q++) {
			sale(product->selectId.get(), saleData->device.get(), saleData->priceList, product->price);
		}
	}
}

MemoryResult Config3Automat::sale(const char *selectId, const char *device, uint16_t number, uint32_t value) {
	if(strcmp("TA", device) == 0) {
		LOG_ERROR(LOG_CFG, "Ignore token sale");
		return MemoryResult_Ok;
	}
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
	return products.sale(productIndex, priceIndex, value);
}

MemoryResult Config3Automat::sale(uint16_t selectId, const char *device, uint16_t number, uint32_t value) {
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
	return products.sale(productIndex, priceIndex, value);
}
