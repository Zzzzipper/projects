#ifndef COMMON_CONFIG_V1_AUTOMAT_H_
#define COMMON_CONFIG_V1_AUTOMAT_H_

#include "Config1PriceIndex.h"
#include "Config1ProductIndex.h"
#include "Config1ProductList.h"
#include "mdb/master/coin_changer/MdbCoin.h"
#include "mdb/master/bill_validator/MdbBill.h"

class Config1Automat {
public:
	enum PaymentBus {
		PaymentBus_None			= 0,
		PaymentBus_MdbMaster	= 1,
		PaymentBus_ExeSlave		= 2,
		PaymentBus_ExeMaster	= 3,
		PaymentBus_MdbSlave		= 4,
		PaymentBus_MaxValue		= 4,
	};

	Config1Automat();
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
	void setDecimalPoint(uint16_t decimalPoint);
	uint16_t getDecimalPoint() const { return decimalPoint; }
	uint32_t convertValueToMoney(uint32_t value) const;
	uint32_t convertMoneyToValue(uint32_t value) const;

	bool addProduct(const char *selectId);
	bool addProduct(const char *selectId, uint16_t cashlessId);
	void addPriceList(const char *device, uint8_t number);

	uint32_t getTotalCount() { return products.getTotalCount(); }
	uint32_t getTotalMoney() { return products.getTotalMoney(); }
	uint32_t getCount() { return products.getCount(); }
	uint32_t getMoney() { return products.getMoney(); }
	uint16_t getProductNum() { return productIndexes.getSize(); }
	uint16_t getPriceListNum() { return priceIndexes.getSize(); }
	Config1PriceIndexList *getPriceIndexList() { return &priceIndexes; }
	Config1ProductIndexList *getProductIndexList() { return &productIndexes; }
	Config1ProductList *getProducts() { return &products; }

private:
	uint32_t automatId;
	uint32_t configId;
	uint16_t paymentBus;
	uint16_t taxSystem;
	uint16_t decimalPoint;
	uint32_t devider;
	Config1PriceIndexList priceIndexes;
	Config1ProductIndexList productIndexes;
	Config1ProductList products;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData(Memory *memory);
	static uint32_t calcDevider(uint16_t decimalPoint);
};

#endif
