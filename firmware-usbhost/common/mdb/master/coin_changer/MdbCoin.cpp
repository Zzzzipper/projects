#include "MdbCoin.h"

#include "common.h"
#include "mdb/MdbProtocolCoinChanger.h"
#include "logger/include/Logger.h"

#define COIN_MAX_NUM 16
#define MDBCC_MINIMAL_CHANGE 100

MdbCoin::MdbCoin() :
	nominal(0),
	inTube(false),
	number(0)
{
}

void MdbCoin::setNominal(uint32_t nominal) {
	this->nominal = nominal;
}

void MdbCoin::setInTube(bool value) {
	this->inTube = value;
}

void MdbCoin::setFullTube(bool value) {
	this->fullTube = value;
}

void MdbCoin::setNumber(uint8_t number) {
	this->number = number;
}

uint32_t MdbCoin::getNominal() {
	return this->nominal;
}

bool MdbCoin::getInTube() {
	return this->inTube;
}

bool MdbCoin::getFullTube() {
	return this->fullTube;
}

uint8_t MdbCoin::getNumber() {
	return this->number;
}

MdbCoinChangerContext::MdbCoinChangerContext(uint32_t masterDecimalPoint, RealTimeInterface *realtime) :
	Mdb::DeviceContext(masterDecimalPoint, realtime),
	realtime(realtime),
	inTubeValue(0),
	lastCoinValue(0)
{
	this->coins = new MdbCoin[COIN_MAX_NUM];
	this->init(0, masterDecimalPoint, 1, 0, 0, 0);
	this->resetChanged();
	this->minimalChange = convertDecimalPoint(0, masterDecimalPoint, MDBCC_MINIMAL_CHANGE);
}

MdbCoinChangerContext::~MdbCoinChangerContext() {
	delete this->coins;
}

void MdbCoinChangerContext::init(uint16_t deviceDecimalPoint, uint16_t scalingFactor) {
	converter.setDeviceDecimalPoint(deviceDecimalPoint);
	converter.setScalingFactor(scalingFactor);
	for(uint16_t i = 0; i < COIN_MAX_NUM; i++) {
		coins[i].setNominal(0);
		coins[i].setInTube(false);
		coins[i].setFullTube(false);
		coins[i].setNumber(0);
	}
}

void MdbCoinChangerContext::init(uint8_t level, uint16_t deviceDecimalPoint, uint16_t scalingFactor, uint8_t *data, uint16_t num, uint16_t coinTypeRouting) {
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
		uint32_t nominal = MDB_CC_TOKEN;
		if(data[i] != MDB_CC_TOKEN_VALUE) {
			nominal = value2money(data[i]);
		}
		if(coins[i].getNominal() != nominal) {
			coins[i].setNominal(nominal);
			this->changed = true;
		}
		if(coinTypeRouting & MASK(i)) {
			coins[i].setInTube(true);
		} else {
			coins[i].setInTube(false);
		}
	}
	for(; i < COIN_MAX_NUM; i++) {
		if(coins[i].getNominal() != 0) {
			coins[i].setNominal(0);
			this->changed = true;
		}
	}
}

uint32_t MdbCoinChangerContext::value2money(uint32_t value) {
	if(value == MDB_CC_TOKEN) {
		return MDB_CC_TOKEN;
	}
	return Mdb::DeviceContext::value2money(value);
}

uint32_t MdbCoinChangerContext::money2value(uint32_t value) {
	if(value == MDB_CC_TOKEN) {
		return MDB_CC_TOKEN;
	}
	return Mdb::DeviceContext::money2value(value);
}

void MdbCoinChangerContext::resetChanged() {
	this->changed = false;
}

bool MdbCoinChangerContext::getChanged() {
	return changed;
}

void MdbCoinChangerContext::update(uint16_t status, uint8_t *data, uint16_t num) {
	uint16_t i = 0;
	inTubeValue = 0;
	for(; i < num; i++) {
		this->coins[i].setFullTube(status & MASK(i));
		this->coins[i].setNumber(data[i]);
		inTubeValue += this->coins[i].getNominal() * this->coins[i].getNumber();
	}
	for(; i < COIN_MAX_NUM; i++) {
		this->coins[i].setNumber(0);
	}
}

void MdbCoinChangerContext::updateTube(uint16_t coinIndex, uint8_t coinNumber) {
	MdbCoin *coin = get(coinIndex);
	if(coin == NULL) {
		return;
	}
	coin->setNumber(coinNumber);
	inTubeValue = 0;
	for(uint16_t i = 0; i < COIN_MAX_NUM; i++) {
		inTubeValue += this->coins[i].getNominal() * this->coins[i].getNumber();
	}
}

void MdbCoinChangerContext::setInTubeValue(uint32_t value) {
	inTubeValue = value;
}

uint32_t MdbCoinChangerContext::getInTubeValue() {
	return inTubeValue;
}

MdbCoin *MdbCoinChangerContext::get(uint16_t index) {
	if(index >= COIN_MAX_NUM) {
		return NULL;
	}
	return &this->coins[index];
}

uint8_t MdbCoinChangerContext::getSize() {
	return COIN_MAX_NUM;
}

uint16_t MdbCoinChangerContext::getMask() {
	uint16_t mask = 0;
	for(uint16_t i = 0; i < COIN_MAX_NUM; i++) {
		if(this->coins[i].getNominal() > 0) {
			mask |= MASK(i);
		}
	}
	return mask;
}

uint16_t MdbCoinChangerContext::getFullMask() {
	uint16_t mask = 0;
	for(uint16_t i = 0; i < COIN_MAX_NUM; i++) {
		MdbCoin *coin = &coins[i];
		if(coin->getNominal() > 0 && coin->getInTube() == true && coin->getFullTube() == true) {
			mask |= MASK(i);
		}
	}
	return mask;
}

uint16_t MdbCoinChangerContext::getNotFullMask() {
	uint16_t mask = 0;
	for(uint16_t i = 0; i < COIN_MAX_NUM; i++) {
		MdbCoin *coin = &coins[i];
		if(coin->getNominal() > 0 && coin->getInTube() == true && coin->getFullTube() == false) {
			mask |= MASK(i);
		}
	}
	return mask;	
}

uint16_t MdbCoinChangerContext::getInTubeMask() {
	uint16_t mask = 0;
	for(uint16_t i = 0; i < COIN_MAX_NUM; i++) {
		if(this->coins[i].getInTube() == true) {
			mask |= MASK(i);
		}
	}
	return mask;
}

bool MdbCoinChangerContext::hasChange() {
	return (inTubeValue > minimalChange);
}

uint8_t MdbCoinChangerContext::getLevel() {
	return level;
}

void MdbCoinChangerContext::registerLastCoin(uint32_t value) {
	realtime->getDateTime(&lastCoinDatetime);
	lastCoinValue = value;
}

DateTime *MdbCoinChangerContext::getLastCoinDatetime() {
	return &lastCoinDatetime;
}

uint32_t MdbCoinChangerContext::getLastCoinValue() {
	return lastCoinValue;
}
