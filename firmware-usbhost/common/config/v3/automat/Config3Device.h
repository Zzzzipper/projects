#ifndef COMMON_CONFIG_V3_DEVICE_H_
#define COMMON_CONFIG_V3_DEVICE_H_

#include "memory/include/Memory.h"
#include "mdb/master/coin_changer/MdbCoin.h"
#include "mdb/master/bill_validator/MdbBill.h"

class Config3DeviceList {
public:
	Config3DeviceList(uint32_t masterDecimalPoint, RealTimeInterface *realtime, uint32_t maxCredit);
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	void setMasterDecimalPoint(uint32_t masterDecimalPoint);
	MdbCoinChangerContext *getCCContext();
	MdbBillValidatorContext *getBVContext();
	Mdb::DeviceContext *getCL1Context();
	Mdb::DeviceContext *getCL2Context();

private:
	Memory *memory;
	uint32_t address;
	MdbCoinChangerContext ccContext;
	MdbBillValidatorContext bvContext;
	Mdb::DeviceContext cl1Context;
	Mdb::DeviceContext cl2Context;
};

#endif
