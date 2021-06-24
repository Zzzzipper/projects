#ifndef COMMON_CONFIG_V2_AUTOMAT_H_
#define COMMON_CONFIG_V2_AUTOMAT_H_

#include "config/v3/automat/Config3PriceIndex.h"
#include "config/v3/automat/Config3ProductIndex.h"
#include "config/v3/automat/Config3ProductIterator.h"
#include "config/v2/automat/Config2ProductList.h"

class Config2Automat {
public:
	enum PaymentBus {
		PaymentBus_None			= 0,
		PaymentBus_MdbMaster	= 1,
		PaymentBus_ExeSlave		= 2,
		PaymentBus_ExeMaster	= 3,
		PaymentBus_MdbSlave		= 4,
		PaymentBus_MaxValue		= 4,
	};

	Config2Automat();
	void setDefault();
	void init();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

	void setAutomatId(uint32_t m_configId);
	uint32_t getAutomatId() const { return automatId; }
	void setConfigId(uint32_t configId);
	uint32_t getConfigId() const { return configId; }
	void setPaymentBus(uint16_t paymentBus);
	uint16_t getPaymentBus() const;
	void setTaxSystem(uint16_t taxSystem);
	uint16_t getTaxSystem() const;
	void setCurrency(uint16_t currency);
	uint16_t getCurrency() const;
	void setDecimalPoint(uint16_t decimalPoint);
	uint16_t getDecimalPoint() const { return decimalPoint; }
	uint32_t convertValueToMoney(uint32_t value) const;
	uint32_t convertMoneyToValue(uint32_t value) const;
	void setMaxCredit(uint32_t maxCredit);
	uint32_t getMaxCredit() const;

	bool addProduct(const char *selectId, uint16_t cashlessId);
	void addPriceList(const char *device, uint8_t number, Config3PriceIndexType type);

	Config3PriceIndexList *getPriceIndexList() { return &priceIndexes; }
	Config3ProductIndexList *getProductIndexList() { return &productIndexes; }
	Config2ProductList *getProductList() { return &products; }

private:
	uint32_t automatId;
	uint32_t configId;
	uint16_t paymentBus;
	uint16_t taxSystem;
	uint16_t currency;
	uint32_t maxCredit;
	bool changeWithoutSale;
	bool lockSaleByFiscal;
	bool freeSale;
	uint16_t lotteryPeriod;
	TimeTable lotteryTime;
	uint16_t decimalPoint;
	uint32_t devider;
	Config3PriceIndexList priceIndexes;
	Config3ProductIndexList productIndexes;
	Config2ProductList products;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData(Memory *memory);
	static uint32_t calcDevider(uint16_t decimalPoint);
};

#endif
