#include "Config4AutomatData.h"
#include "Config4Automat.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CONFIG4_AUTOMAT_VERSION 4
#define MAX_CREDIT_DEFAULT 50000
#define CASHLESS1_NUM 1
#define CASHLESS2_NUM 2
#define SCALE_FACTOR_MIN 1
#define SCALE_FACTOR_MAX 255

enum Config4AutomatFlag {
	Config4AutomatFlag_Evadts			 = 0x01,
	Config4AutomatFlag_CreditHolding	 = 0x02,
	Config4AutomatFlag_FreeSale			 = 0x04,
	Config4AutomatFlag_CategoryMoney	 = 0x08,
	Config4AutomatFlag_ShowChange		 = 0x10,
	Config4AutomatFlag_PriceHolding		 = 0x20,
	Config4AutomatFlag_MultiVend		 = 0x40,
	Config4AutomatFlag_Cashless2Click	 = 0x80,
};

#pragma pack(push,1)
struct Config4AutomatStruct {
	uint8_t  version;
	uint32_t automatId;
	uint32_t configId;
	uint16_t paymentBus;
	uint16_t taxSystem;
	uint16_t decimalPoint;
	uint32_t maxCredit;
	uint16_t currency;
	uint8_t  flags;
	uint8_t  ext1Device;
	uint8_t  freeSale;
	uint8_t  cashlessNum;
	uint8_t  scaleFactor;
	uint8_t  cashlessMaxLevel;
	uint8_t  qrCode;
	uint8_t  reserved[89];
	uint16_t lotteryPeriod;
	uint8_t  lotteryWeek;
	uint8_t  lotteryHour;
	uint8_t  lotteryMinute;
	uint8_t  lotterySecond;
	uint32_t lotteryInterval;
	uint8_t  crc[1];
};
#pragma pack(pop)

Config4AutomatData::Config4AutomatData(RealTimeInterface *realtime) :
	smContext(DECIMAL_POINT_DEFAULT, realtime)
{
	setDefault();
	uint32_t size = sizeof(Config4AutomatStruct);
}

void Config4AutomatData::setDefault() {
	flags = 0;
	setConfigId(0);
	setAutomatId(0);
	setPaymentBus(Config4Automat::PaymentBus_MdbMaster);
	setDecimalPoint(DECIMAL_POINT_DEFAULT);
	setTaxSystem(Fiscal::TaxSystem_ENVD);
	setCurrency(RUSSIAN_CURRENCY_RUB);
	setMaxCredit(MAX_CREDIT_DEFAULT);
	setExt1Device(0);
	setCashlessNumber(2);
	setCashlessMaxLevel(Mdb::FeatureLevel_1);
	setScaleFactor(1);
	setCategoryMoney(false);
	setShowChange(true);
	setQrCode(0);
}

/**
 * TODO: Костыль для сохранения значения двоичной точки при инициализации.
 * Видимо метафора кривая и значение двоичной точки должно принадлежать другой сущности.
 */
MemoryResult Config4AutomatData::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	uint16_t decimalPoint = smContext.getDecimalPoint();
	setDefault();
	setDecimalPoint(decimalPoint);
	return save();
}

MemoryResult Config4AutomatData::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	Config4AutomatStruct data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong automat config CRC");
		return result;
	}
	if(data.version != CONFIG4_AUTOMAT_VERSION) {
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
	setExt1Device(data.ext1Device);
	this->flags = data.flags;
	setCashlessNumber(data.cashlessNum);
	setCashlessMaxLevel(data.cashlessMaxLevel);
	setScaleFactor(data.scaleFactor);
	setQrCode(data.qrCode);
	return MemoryResult_Ok;
}

MemoryResult Config4AutomatData::save() {
	memory->setAddress(address);
	Config4AutomatStruct data;
	data.version = CONFIG4_AUTOMAT_VERSION;
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
	data.qrCode = qrCode;
	memset(data.reserved, 0, sizeof(data.reserved));
	data.lotteryPeriod = 0;
	data.lotteryWeek = 0;
	data.lotteryHour = 0;
	data.lotteryMinute = 0;
	data.lotterySecond = 0;
	data.lotteryInterval = 0;

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

Mdb::DeviceContext *Config4AutomatData::getContext() {
	return &smContext;
}

void Config4AutomatData::setAutomatId(uint32_t automatId) {
	this->automatId = automatId;
}

uint32_t Config4AutomatData::getAutomatId() const {
	return automatId;
}

void Config4AutomatData::setConfigId(uint32_t configId) {
	this->configId = configId;
}

uint32_t Config4AutomatData::getConfigId() const {
	return configId;
}

void Config4AutomatData::setPaymentBus(uint16_t paymentBus) {
	if(paymentBus > Config4Automat::PaymentBus_MaxValue) {
		LOG_ERROR(LOG_CFG, "Wrong payment bus value " << paymentBus);
		return;
	}
	this->paymentBus = paymentBus;
}

uint16_t Config4AutomatData::getPaymentBus() const {
	return paymentBus;
}

void Config4AutomatData::setEvadts(bool evadts) {
	if(evadts > 0) {
		this->flags |= Config4AutomatFlag_Evadts;
	} else {
		this->flags &= ~Config4AutomatFlag_Evadts;
	}
}

bool Config4AutomatData::getEvadts() const {
	return (flags & Config4AutomatFlag_Evadts) > 0;
}

void Config4AutomatData::setTaxSystem(uint16_t taxSystem) {
	this->taxSystem = taxSystem;
}

uint16_t Config4AutomatData::getTaxSystem() const {
	return taxSystem;
}

void Config4AutomatData::setCurrency(uint16_t currency) {
	if(currency != RUSSIAN_CURRENCY_RUB && currency != RUSSIAN_CURRENCY_RUR) {
		LOG_ERROR(LOG_CFG, "Wrong currency value " << currency);
		currency = RUSSIAN_CURRENCY_RUB;
	}
	this->smContext.setCurrency(currency);
}

uint16_t Config4AutomatData::getCurrency() const {
	return smContext.getCurrency();
}

void Config4AutomatData::setDecimalPoint(uint16_t decimalPoint) {
	if(decimalPoint > DECIMAL_POINT_MAX) {
		LOG_ERROR(LOG_CFG, "Wrong decimal point value " << paymentBus);
		return;
	}
	this->smContext.init(decimalPoint, smContext.getScalingFactor());
	this->smContext.setMasterDecimalPoint(decimalPoint);
}

uint16_t Config4AutomatData::getDecimalPoint() const {
	return smContext.getDecimalPoint();
}

void Config4AutomatData::setMaxCredit(uint32_t maxCredit) {
	this->maxCredit = maxCredit;
}

uint32_t Config4AutomatData::getMaxCredit() const {
	return maxCredit;
}

void Config4AutomatData::setExt1Device(uint8_t ext1Device) {
	this->ext1Device = ext1Device;
}

uint8_t Config4AutomatData::getExt1Device() const {
	return ext1Device;
}

void Config4AutomatData::setCashlessNumber(uint8_t cashlessNum) {
	if(cashlessNum != CASHLESS1_NUM && cashlessNum != CASHLESS2_NUM) {
		return;
	}
	this->cashlessNum = cashlessNum;
}

uint8_t Config4AutomatData::getCashlessNumber() const {
	return cashlessNum;
}

void Config4AutomatData::setCashlessMaxLevel(uint8_t cashlessMaxLevel) {
	if(cashlessMaxLevel < Mdb::FeatureLevel_1 || cashlessMaxLevel > Mdb::FeatureLevel_3) {
		return;
	}
	this->cashlessMaxLevel = cashlessMaxLevel;
}

uint8_t Config4AutomatData::getCashlessMaxLevel() {
	return cashlessMaxLevel;
}

void Config4AutomatData::setScaleFactor(uint32_t scaleFactor) {
	if(scaleFactor < SCALE_FACTOR_MIN || scaleFactor > SCALE_FACTOR_MAX) {
		return;
	}
	smContext.init(smContext.getDecimalPoint(), scaleFactor);
}

uint8_t Config4AutomatData::getScaleFactor() const {
	return smContext.getScalingFactor();
}

void Config4AutomatData::setQrCode(uint32_t qrCode) {
	if(qrCode > 3) {
		return;
	}
	this->qrCode = qrCode;
}

uint8_t Config4AutomatData::getQrCode() {
	return qrCode;
}

void Config4AutomatData::setPriceHolding(bool priceHolding) {
	if(priceHolding > 0) {
		this->flags |= Config4AutomatFlag_PriceHolding;
	} else {
		this->flags &= ~Config4AutomatFlag_PriceHolding;
	}
}

bool Config4AutomatData::getPriceHolding() const {
	return (flags & Config4AutomatFlag_PriceHolding) > 0;
}

void Config4AutomatData::setCategoryMoney(bool categoryMoney) {
	if(categoryMoney > 0) {
		this->flags |= Config4AutomatFlag_CategoryMoney;
	} else {
		this->flags &= ~Config4AutomatFlag_CategoryMoney;
	}
}

bool Config4AutomatData::getCategoryMoney() const {
	return (flags & Config4AutomatFlag_CategoryMoney) > 0;
}

void Config4AutomatData::setShowChange(bool showChange) {
	if(showChange > 0) {
		this->flags |= Config4AutomatFlag_ShowChange;
	} else {
		this->flags &= ~Config4AutomatFlag_ShowChange;
	}
}

bool Config4AutomatData::getShowChange() const {
	return (flags & Config4AutomatFlag_ShowChange) > 0;
}

void Config4AutomatData::setCreditHolding(bool creditHolding) {
	if(creditHolding > 0) {
		this->flags |= Config4AutomatFlag_CreditHolding;
	} else {
		this->flags &= ~Config4AutomatFlag_CreditHolding;
	}
}

bool Config4AutomatData::getCreditHolding() const {
	return (flags & Config4AutomatFlag_CreditHolding) > 0;
}

void Config4AutomatData::setMultiVend(bool multiVend) {
	if(multiVend > 0) {
		this->flags |= Config4AutomatFlag_MultiVend;
	} else {
		this->flags &= ~Config4AutomatFlag_MultiVend;
	}
}

bool Config4AutomatData::getMultiVend() const {
	return (flags & Config4AutomatFlag_MultiVend) > 0;
}

void Config4AutomatData::setCashless2Click(bool cashless2Click) {
	if(cashless2Click > 0) {
		this->flags |= Config4AutomatFlag_Cashless2Click;
	} else {
		this->flags &= ~Config4AutomatFlag_Cashless2Click;
	}
}

bool Config4AutomatData::getCashless2Click() {
	return (flags & Config4AutomatFlag_Cashless2Click) > 0;
}
