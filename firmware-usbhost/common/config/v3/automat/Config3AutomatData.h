#ifndef COMMON_CONFIG3_AUTOMATDATA_H_
#define COMMON_CONFIG3_AUTOMATDATA_H_

#include "utils/include/NetworkProtocol.h"
#include "evadts/EvadtsProtocol.h"

#define CONFIG3_AUTOMAT_VERSION 3

enum ConfigAutomatFlag {
	ConfigAutomatFlag_Evadts		 = 0x01,
	ConfigAutomatFlag_CreditHolding	 = 0x02,
	ConfigAutomatFlag_FreeSale		 = 0x04,
	ConfigAutomatFlag_CategoryMoney	 = 0x08,
	ConfigAutomatFlag_ShowChange	 = 0x10,
	ConfigAutomatFlag_PriceHolding	 = 0x20,
	ConfigAutomatFlag_MultiVend		 = 0x40,
	ConfigAutomatFlag_Cashless2Click = 0x80,
};

#pragma pack(push,1)
#if 0
struct Config3AutomatStruct {
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
#else
struct Config3AutomatStruct {
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
	uint8_t  fiscalPrinter;
	uint8_t  internetDevice;
	uint8_t  ext2Device;
	uint8_t  usb1Device;
	uint8_t  qrType;
	uint8_t  bvOn;
	uint8_t  reserved[9];
	uint8_t  crc[1];
};
#endif

struct Config3DeviceStruct {
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

struct Config3ProductListData {
	uint16_t productNum;
	uint16_t priceListNum;
	uint32_t totalCount;
	uint32_t totalMoney;
	uint32_t count;
	uint32_t money;
	uint8_t  crc[1];
};

struct Config3ProductData {
	uint8_t  enable;
	uint32_t wareId;
	StrParam<ConfigProductNameSize> name;
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

struct Config3PriceData {
	uint32_t price;
	uint32_t totalCount;
	uint32_t totalMoney;
	uint32_t count;
	uint32_t money;
	uint8_t  crc[1];
};

#pragma pack(pop)

#endif
