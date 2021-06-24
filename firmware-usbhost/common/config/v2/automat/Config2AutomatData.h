#ifndef COMMON_CONFIG_AUTOMATDATA2_H_
#define COMMON_CONFIG_AUTOMATDATA2_H_

#include "utils/include/NetworkProtocol.h"
#include "evadts/EvadtsProtocol.h"

#define CONFIG_AUTOMAT_VERSION2 2
#define ConfigProductNameSize2 50
#define MAX_CREDIT_DEFAULT 50000

#pragma pack(push,1)
struct Config2AutomatStruct {
	uint8_t  version;
	uint32_t automatId;
	uint32_t configId;
	uint16_t paymentBus;
	uint16_t taxSystem;
	uint16_t decimalPoint;
	uint32_t maxCredit;
	uint16_t currency;
	uint8_t  changeWithoutSale;
	uint8_t  lockSaleByFiscal;
	uint8_t  freeSale;
	uint8_t  workWeek;
	uint8_t  workHour;
	uint8_t  workMinute;
	uint8_t  workSecond;
	uint32_t workInterval;
	uint16_t lotteryPeriod;
	uint8_t  lotteryWeek;
	uint8_t  lotteryHour;
	uint8_t  lotteryMinute;
	uint8_t  lotterySecond;
	uint32_t lotteryInterval;
	uint8_t  crc[1];
};

struct Config2ProductListData {
	uint16_t productNum;
	uint16_t priceListNum;
	uint32_t totalCount;
	uint32_t totalMoney;
	uint32_t count;
	uint32_t money;
	uint8_t  crc[1];
};

struct Config2ProductData {
	uint8_t  enable;
	uint32_t wareId;
	StrParam<ConfigProductNameSize2> name;
	uint16_t taxRate;
	uint32_t totalCount;
	uint32_t totalMoney;
	uint32_t count;
	uint32_t money;
	uint32_t testTotalCount;
	uint32_t testCount;
	uint32_t freeTotalCount;
	uint32_t freeCount;
	uint8_t  crc[1];
};

struct Config2PriceData {
	uint32_t price;
	uint32_t totalCount;
	uint32_t totalMoney;
	uint32_t count;
	uint32_t money;
	uint8_t  crc[1];
};

#pragma pack(pop)

#endif
