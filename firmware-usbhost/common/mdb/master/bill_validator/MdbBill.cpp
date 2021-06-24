#include "MdbBill.h"

#include "mdb/MdbProtocolBillValidator.h"
#include "platform/include/platform.h"
#include "logger/include/Logger.h"

#define BILL_MAX_NUM 16
#define MASK2(x) (1 << (x)) //todo: объединить с MASK из common/common.h и продумать где правильно место для этих макросов

void MdbBill::setNominal(uint32_t nominal) {
	this->nominal = nominal;
}

uint32_t MdbBill::getNominal() {
	return this->nominal;
}

MdbBillValidatorContext::MdbBillValidatorContext(uint32_t masterDecimalPoint, RealTimeInterface *realtime, uint32_t maxCredit) :
	Mdb::DeviceContext(masterDecimalPoint, realtime),
	realtime(realtime),
	useChange(false),
	billInStacker(BV_STACKER_UNKNOWN),
	lastBillValue(0)
{
	this->bills = new MdbBill[BILL_MAX_NUM];
	this->init(0, masterDecimalPoint, 1, 0, 0);
	this->setMaxBill(maxCredit);
	this->resetChanged();
}

MdbBillValidatorContext::~MdbBillValidatorContext() {
	delete this->bills;
}

void MdbBillValidatorContext::init(uint8_t level, uint32_t deviceDecimalPoint, uint32_t scalingFactor, uint8_t *data, uint8_t num) {
	if(this->level != level) {
		this->level = level;
		this->changed = true;
	}
	if(this->converter.getDeviceDecimalPoint() != deviceDecimalPoint) {
		this->converter.setDeviceDecimalPoint(deviceDecimalPoint);
		this->changed = true;
	}
	if(this->converter.getScalingFactor() != scalingFactor) {
		this->converter.setScalingFactor(scalingFactor);
		this->changed = true;
	}
	uint16_t i = 0;
	for(; i < num; i++) {
		uint32_t nominal = value2money(data[i]);
		if(bills[i].getNominal() != nominal) {
			bills[i].setNominal(nominal);
			this->changed = true;
		}
	}
	for(; i < BILL_MAX_NUM; i++) {
		if(bills[i].getNominal() != 0) {
			bills[i].setNominal(0);
			this->changed = true;
		}
	}
}

void MdbBillValidatorContext::resetChanged() {
	this->changed = false;
}

bool MdbBillValidatorContext::getChanged() {
	return changed;
}

uint16_t MdbBillValidatorContext::getMask() {
	uint16_t mask = 0;
	for(uint16_t i = 0; i < BILL_MAX_NUM; i++) {
		uint32_t nominal = bills[i].getNominal();
		if(0 < nominal && nominal <= maxBill && (useChange == false || nominal <= maxChange)) {
			mask |= MASK2(i);
		}
	}
	return mask;
}

uint32_t MdbBillValidatorContext::getBillNominal(uint8_t index) {
	if(index >= BILL_MAX_NUM) {
		return 0;
	}
	return bills[index].getNominal();
}

uint8_t MdbBillValidatorContext::getBillValue(uint8_t index) {
	if(index >= BILL_MAX_NUM) {
		return 0;
	}
	uint32_t nominal = bills[index].getNominal();
	return money2value(nominal);
}

uint8_t MdbBillValidatorContext::getBillNumber() {
	return BILL_MAX_NUM;
}

uint8_t MdbBillValidatorContext::getLevel() {
	return level;
}

void MdbBillValidatorContext::setUseChange(bool value) {
	this->useChange = value;
}

bool MdbBillValidatorContext::getUseChange() {
	return useChange;
}

void MdbBillValidatorContext::setMaxChange(uint32_t maxChange) {
	this->maxChange = maxChange;
}

void MdbBillValidatorContext::setMaxBill(uint32_t maxBill) {
	this->maxBill = maxBill;
}

uint32_t MdbBillValidatorContext::getMaxBill() {
	return maxBill;
}

void MdbBillValidatorContext::setBillInStacker(uint16_t billInStacker) {
	this->billInStacker = billInStacker;
}

uint16_t MdbBillValidatorContext::getBillInStacker() {
	return billInStacker;
}

void MdbBillValidatorContext::registerLastBill(uint32_t value) {
	realtime->getDateTime(&lastBillDatetime);
	lastBillValue = value;
}

DateTime *MdbBillValidatorContext::getLastBillDatetime() {
	return &lastBillDatetime;
}

uint32_t MdbBillValidatorContext::getLastBillValue() {
	return lastBillValue;
}
