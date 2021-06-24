#include "Config3Device.h"
#include "Config3AutomatData.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

Config3DeviceList::Config3DeviceList(uint32_t masterDecimalPoint, RealTimeInterface *realtime, uint32_t maxCredit) :
	ccContext(masterDecimalPoint, realtime),
	bvContext(masterDecimalPoint, realtime, maxCredit),
	cl1Context(masterDecimalPoint, realtime),
	cl2Context(masterDecimalPoint, realtime)
{
}

MemoryResult Config3DeviceList::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	return save();
}

MemoryResult Config3DeviceList::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	Config3DeviceStruct data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong device config CRC");
		return result;
	}

	ccContext.init(data.ccLevel, data.ccDecimalPoint, data.ccScaleFactor, data.ccCoins, sizeof(data.ccCoins), data.ccCoinRouting);
	ccContext.resetChanged();
	bvContext.init(data.bvLevel, data.bvDecimalPoint, data.bvScaleFactor, data.bvBills, sizeof(data.bvBills));
	bvContext.resetChanged();
	return MemoryResult_Ok;
}

MemoryResult Config3DeviceList::save() {
	memory->setAddress(address);

	Config3DeviceStruct data;
	data.ccLevel = ccContext.getLevel();
	data.ccDecimalPoint = ccContext.getDecimalPoint();
	data.ccScaleFactor = ccContext.getScalingFactor();
	for(uint16_t i = 0; i < ccContext.getSize(); i++) {
		MdbCoin *coin = ccContext.get(i);
		data.ccCoins[i] = ccContext.money2value(coin->getNominal());
	}
	data.ccCoinRouting = ccContext.getInTubeMask();

	data.bvLevel = bvContext.getLevel();
	data.bvDecimalPoint = bvContext.getDecimalPoint();
	data.bvScaleFactor = bvContext.getScalingFactor();
	for(uint16_t i = 0; i < bvContext.getBillNumber(); i++) {
		data.bvBills[i] = bvContext.getBillValue(i);
	}

	data.cl1Level = 0;
	data.cl1DecimalPoint = 0;
	data.cl1ScaleFactor = 0;

	data.cl2Level = 0;
	data.cl2DecimalPoint = 0;
	data.cl2ScaleFactor = 0;

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config3DeviceList::setMasterDecimalPoint(uint32_t masterDecimalPoint) {
	ccContext.setMasterDecimalPoint(masterDecimalPoint);
	bvContext.setMasterDecimalPoint(masterDecimalPoint);
	cl1Context.setMasterDecimalPoint(masterDecimalPoint);
	cl2Context.setMasterDecimalPoint(masterDecimalPoint);
}

MdbCoinChangerContext *Config3DeviceList::getCCContext() {
	return &ccContext;
}

MdbBillValidatorContext *Config3DeviceList::getBVContext() {
	return &bvContext;
}

Mdb::DeviceContext *Config3DeviceList::getCL1Context() {
	return &cl1Context;
}

Mdb::DeviceContext *Config3DeviceList::getCL2Context() {
	return &cl2Context;
}
