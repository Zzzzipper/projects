#ifndef COMMON_CONFIG_V4_AUTOMAT_H_
#define COMMON_CONFIG_V4_AUTOMAT_H_

#include "config/v4/automat/Config4AutomatData.h"
#include "config/v4/automat/Config4PaymentStat.h"
#include "config/v4/automat/Config4DeviceList.h"
#include "config/v3/automat/Config3PriceIndex.h"
#include "config/v3/automat/Config3ProductIndex.h"
#include "config/v4/automat/Config4ProductIterator.h"
#include "config/v4/automat/Config4ProductList.h"

class Config4Automat {
public:
	enum PaymentBus {
		PaymentBus_None			= 0,
		PaymentBus_MdbMaster	= 1,
		PaymentBus_ExeSlave		= 2,
		PaymentBus_ExeMaster	= 3,
		PaymentBus_MdbSlave		= 4,
		PaymentBus_CciT4		= 5,
		PaymentBus_CciT3		= 6,
		PaymentBus_MdbDumb		= 7,
		PaymentBus_CciF1		= 8,
		PaymentBus_OrderCciT3	= 9,
		PaymentBus_OrderSpire	= 10,
		PaymentBus_MaxValue		= 10,
	};
	enum InternetDevice {
		InternetDevice_None		= 0x80,
		InternetDevice_Gsm		= 0x01,
		InternetDevice_Ethernet = 0x02,
	};
	enum Device {
		Device_None				= 0,
		Device_Inpas			= 2,
		Device_Sberbank			= 3,
		Device_Vendotek			= 4,
		Device_Screen			= 5,
		Device_Scanner			= 6,
	};
	enum QrType {
		QrType_None				= 0x00,
		QrType_Free				= 0x01,
		QrType_Unitex1			= 0x02,
		QrType_EphorOrder		= 0x04,
		QrType_NefteMag			= 0x08,
	};
	enum FiscalPrinter {
		FiscalPrinter_None		= 0x00,
		FiscalPrinter_Cashless	= 0x01,
		FiscalPrinter_Screen	= 0x02,
	};

	Config4Automat(RealTimeInterface *realtime);
	void shutdown(); //todo: переименовать, плохая метафора
	MemoryResult init(Memory *memory);
	void asyncInitStart(Memory *memory);
	bool asyncInitProc(Memory *memory);
	MemoryResult reinit(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult loadAndCheck(Memory *memory);
	MemoryResult save();
	MemoryResult reset();

	void setAutomatId(uint32_t m_configId);
	uint32_t getAutomatId() const;
	void setConfigId(uint32_t configId);
	uint32_t getConfigId() const;
	void setPaymentBus(uint16_t paymentBus);
	uint16_t getPaymentBus() const;
	void setEvadts(bool value);
	bool getEvadts() const;
	void setTaxSystem(uint16_t taxSystem);
	uint16_t getTaxSystem() const;
	void setCurrency(uint16_t currency);
	uint16_t getCurrency() const;
	void setDecimalPoint(uint16_t decimalPoint);
	uint16_t getDecimalPoint() const;
	void setMaxCredit(uint32_t maxCredit);
	uint32_t getMaxCredit() const;
	void setExt1Device(uint8_t ext1Device);
	uint8_t getExt1Device();
	void setCashlessNumber(uint8_t cashlessNum);
	uint8_t getCashlessNumber();
	void setCashlessMaxLevel(uint8_t cashlessMaxLevel);
	uint8_t getCashlessMaxLevel();
	void setScaleFactor(uint32_t scaleFactor);
	uint8_t getScaleFactor();
	void setQrCode(uint32_t qrCode);
	uint8_t getQrCode();
	void setPriceHolding(bool priceHolding);
	bool getPriceHolding();
	void setCategoryMoney(bool categoryMoney);
	bool getCategoryMoney();
	void setShowChange(bool showChange);
	bool getShowChange();
	void setCreditHolding(bool creditHolding);
	bool getCreditHolding();
	void setMultiVend(bool multiVend);
	bool getMultiVend();
	void setCashless2Click(bool cashless2Click);
	bool getCashless2Click();

	Mdb::DeviceContext *getSMContext();
	MdbCoinChangerContext *getCCContext();
	MdbBillValidatorContext *getBVContext();
	Mdb::DeviceContext *getMdb1CashlessContext();
	Mdb::DeviceContext *getMdb2CashlessContext();
	Mdb::DeviceContext *getExt1CashlessContext();
	Mdb::DeviceContext *getUsb1CashlessContext();
	Fiscal::Context *getFiscalContext();
	Config4DeviceList *getDeviceList();

	const char *getCommunictionId();

	bool addProduct(const char *selectId, uint16_t cashlessId);
	void addPriceList(const char *device, uint8_t number, Config3PriceIndexType type);
	Config4ProductIterator *createIterator();
	MemoryResult sale(const char *selectId, const char *device, uint16_t number, uint32_t value);
	MemoryResult sale(uint16_t cashlessId, const char *device, uint16_t number, uint32_t value);
	MemoryResult registerCoinIn(uint32_t value, uint8_t route);
	MemoryResult registerCoinDispense(uint32_t value);
	MemoryResult registerCoinDispenseManually(uint32_t value);
	MemoryResult registerCoinFill(uint32_t value);
	MemoryResult registerBillIn(uint32_t value);
	MemoryResult registerCashOverpay(uint32_t value);

	uint16_t getProductNum() { return productIndexes.getSize(); }
	uint16_t getPriceListNum() { return priceIndexes.getSize(); }
	Config4PaymentStat *getPaymentStat() { return &paymentStat; }
	Config3ProductIndexList *getProductIndexList() { return &productIndexes; }
	Config3PriceIndexList *getPriceIndexList() { return &priceIndexes; }
	Config4ProductList *getProductList() { return &products; }

private:
	enum Flag {
		Flag_Evadts			 = 0x01,
		Flag_CreditHolding	 = 0x02,
		Flag_FreeSale		 = 0x04,
		Flag_CategoryMoney	 = 0x08,
		Flag_ShowChange		 = 0x10,
		Flag_PriceHolding	 = 0x20,
		Flag_MultiVend		 = 0x40,
	};

	Config4AutomatData data;
	Config4PaymentStat paymentStat;
	Config4DeviceList devices;
	Config3PriceIndexList priceIndexes;
	Config3ProductIndexList productIndexes;
	Config4ProductList products;
};

#endif
