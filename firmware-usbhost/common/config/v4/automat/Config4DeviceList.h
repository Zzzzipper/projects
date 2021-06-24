#ifndef COMMON_CONFIG_V4_DEVICE_H_
#define COMMON_CONFIG_V4_DEVICE_H_

#include "mdb/master/coin_changer/MdbCoin.h"
#include "mdb/master/bill_validator/MdbBill.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "memory/include/Memory.h"

class Config4DeviceList {
public:
	Config4DeviceList(uint32_t masterDecimalPoint, RealTimeInterface *realtime, uint32_t maxCredit);
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	void setMasterDecimalPoint(uint32_t masterDecimalPoint);
	MdbCoinChangerContext *getCCContext();
	MdbBillValidatorContext *getBVContext();
	Mdb::DeviceContext *getCL1Context();
	Mdb::DeviceContext *getCL2Context();
	Mdb::DeviceContext *getCL3Context();
	Mdb::DeviceContext *getCL4Context();
	Fiscal::Context *getFiscalContext();

private:
	Memory *memory;
	uint32_t address;
	MdbCoinChangerContext ccContext;
	MdbBillValidatorContext bvContext;
	Mdb::DeviceContext cl1Context;
	Mdb::DeviceContext cl2Context;
	Mdb::DeviceContext cl3Context;
	Mdb::DeviceContext cl4Context;
	Fiscal::Context fiscalContext;
};

#endif
