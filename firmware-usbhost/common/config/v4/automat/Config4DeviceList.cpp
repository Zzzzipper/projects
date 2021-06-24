#include "Config4DeviceList.h"
#include "Config4AutomatData.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#pragma pack(push,1)
struct Config4DeviceStruct {
	uint8_t  ccLevel;
	uint8_t  ccDecimalPoint;
	uint8_t  ccScaleFactor;
	uint8_t  ccCoins[16];
	uint16_t ccCoinRouting;
	uint8_t  bvLevel;
	uint8_t  bvDecimalPoint;
	uint8_t  bvScaleFactor;
	uint8_t  bvBills[16];
	uint8_t  cl1Level;
	uint8_t  cl1DecimalPoint;
	uint8_t  cl1ScaleFactor;
	uint8_t  cl2Level;
	uint8_t  cl2DecimalPoint;
	uint8_t  cl2ScaleFactor;
	uint8_t  crc[1];
};
#pragma pack(pop)

Config4DeviceList::Config4DeviceList(uint32_t masterDecimalPoint, RealTimeInterface *realtime, uint32_t maxCredit) :
	ccContext(masterDecimalPoint, realtime),
	bvContext(masterDecimalPoint, realtime, maxCredit),
	cl1Context(masterDecimalPoint, realtime),
	cl2Context(masterDecimalPoint, realtime),
	cl3Context(masterDecimalPoint, realtime),
	cl4Context(masterDecimalPoint, realtime),
	fiscalContext(masterDecimalPoint, realtime)
{
}

MemoryResult Config4DeviceList::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	return save();
}

MemoryResult Config4DeviceList::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	Config4DeviceStruct data;
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

MemoryResult Config4DeviceList::save() {
	memory->setAddress(address);

	Config4DeviceStruct data;
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

void Config4DeviceList::setMasterDecimalPoint(uint32_t masterDecimalPoint) {
	ccContext.setMasterDecimalPoint(masterDecimalPoint);
	bvContext.setMasterDecimalPoint(masterDecimalPoint);
	cl1Context.setMasterDecimalPoint(masterDecimalPoint);
	cl2Context.setMasterDecimalPoint(masterDecimalPoint);
	cl3Context.setMasterDecimalPoint(masterDecimalPoint);
	cl4Context.setMasterDecimalPoint(masterDecimalPoint);
	fiscalContext.setMasterDecimalPoint(masterDecimalPoint);
}

MdbCoinChangerContext *Config4DeviceList::getCCContext() {
	return &ccContext;
}

MdbBillValidatorContext *Config4DeviceList::getBVContext() {
	return &bvContext;
}

Mdb::DeviceContext *Config4DeviceList::getCL1Context() {
	return &cl1Context;
}

Mdb::DeviceContext *Config4DeviceList::getCL2Context() {
	return &cl2Context;
}

Mdb::DeviceContext *Config4DeviceList::getCL3Context() {
	return &cl3Context;
}

Mdb::DeviceContext *Config4DeviceList::getCL4Context() {
	return &cl4Context;
}

Fiscal::Context *Config4DeviceList::getFiscalContext() {
	return &fiscalContext;
}
