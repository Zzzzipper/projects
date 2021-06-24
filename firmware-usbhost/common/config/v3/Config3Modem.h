#ifndef COMMON_CONFIG_V3_MODEM_H_
#define COMMON_CONFIG_V3_MODEM_H_

#include "config/v1/boot/Config1Boot.h"
#include "config/v2/fiscal/Config2Fiscal.h"
#include "config/v3/event/Config3EventList.h"
#include "config/v3/automat/Config3Automat.h"
#include "config/include/StatStorage.h"

class Config3Modem {
public:
	Config3Modem(Memory *memory, RealTimeInterface *realtime, StatStorage *stat);
	~Config3Modem();
	MemoryResult init();
	MemoryResult load();
	MemoryResult save();
	MemoryResult reinit();

	Evadts::Result comparePlanogram(Config3PriceIndexList *prices, Config3ProductIndexList *products);
	bool asyncComparePlanogramStart(Config3PriceIndexList *prices, Config3ProductIndexList *products);
	bool asyncComparePlanogramProc();
	Evadts::Result asyncComparePlanogramResult();

	bool isResizeable(Config3PriceIndexList *prices, Config3ProductIndexList *products);

	Evadts::Result resizePlanogram(Config3PriceIndexList *prices, Config3ProductIndexList *products);
	bool asyncResizePlanogramStart(Config3PriceIndexList *prices, Config3ProductIndexList *products);
	bool asyncResizePlanogramProc();
	Evadts::Result asyncResizePlanogramResult();

	Config1Boot *getBoot() { return &boot; }
	Config2Fiscal *getFiscal() { return &fiscal; }
	Config3EventList *getEvents() { return &events; }
	Config3Automat *getAutomat() { return &automat; }
	StatStorage *getStat() { return stat; }
	RealTimeInterface *getRealTime() { return realtime; }

private:
	Memory *memory;
	RealTimeInterface *realtime;
	StatStorage *stat;
	uint32_t address;
	Config1Boot boot;
	Config2Fiscal fiscal;
	Config3EventList events;
	Config3Automat automat;
	uint16_t num;
	Config3ProductIterator *product;
	Config3ProductIndexList *products;
	Evadts::Result result2;
};

#endif
