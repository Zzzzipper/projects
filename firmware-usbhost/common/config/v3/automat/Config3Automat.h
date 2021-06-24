#ifndef COMMON_CONFIG_V3_AUTOMAT_H_
#define COMMON_CONFIG_V3_AUTOMAT_H_

#include "Config3Device.h"
#include "Config3PriceIndex.h"
#include "Config3ProductIndex.h"
#include "Config3ProductIterator.h"
#include "Config3ProductList.h"
#include "Config3AutomatSettings.h"
#include "Config3BarcodeList.h"
#include "Config3WareGroupList.h"
#include "Config3WareLinkList.h"
#include "mdb/master/coin_changer/MdbCoin.h"
#include "mdb/master/bill_validator/MdbBill.h"
#include "fiscal_register/include/FiscalRegister.h"

class Config3Automat {
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
		Device_Dummy			= 1,
		Device_Inpas			= 2,
		Device_Sberbank			= 3,
		Device_Vendotek			= 4,
		Device_Screen			= 5,
		Device_Scanner			= 6,
		Device_Ingenico			= 7,
		Device_Twocan			= 8,
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

	Config3Automat(RealTimeInterface *realtime);
	void setDefault();
	void shutdown(); //todo: переименовать, плохая метафора
	MemoryResult init(Memory *memory);
	void asyncInitStart(Memory *memory);
	bool asyncInitProc();
	MemoryResult reinit(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult loadAndCheck(Memory *memory);
	MemoryResult save();
	MemoryResult reset();

	void setAutomatId(uint32_t automatId);
	uint32_t getAutomatId() const { return automatId; }
	void setConfigId(uint32_t configId);
	uint32_t getConfigId() const { return configId; }
	void setTaxSystem(uint16_t taxSystem);
	uint16_t getTaxSystem() const;

	void setInternetDevice(uint8_t internetDevice);
	uint8_t getInternetDevice();
	void setExt1Device(uint8_t ext1Device);
	uint8_t getExt1Device();
	void setExt2Device(uint8_t ext2Device);
	uint8_t getExt2Device();
	void setUsb1Device(uint8_t usb1Device);
	uint8_t getUsb1Device();
	void setQrType(uint32_t qrType);
	uint8_t getQrType();
	void setFiscalPrinter(uint32_t fiscalPrinter);
	uint8_t getFiscalPrinter();

	void setEthMac(uint8_t *mac);
	void setEthMac(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5);
	uint8_t *getEthMac();
	uint16_t getEthMacLen();
	void setEthAddr(uint32_t addr);
	uint32_t getEthAddr();
	void setEthMask(uint32_t addr);
	uint32_t getEthMask();
	void setEthGateway(uint32_t addr);
	uint32_t getEthGateway();

	void setFidType(uint16_t type);
	uint16_t getFidType();
	void setFidDevice(const char *device);
	const char *getFidDevice();
	void setFidAddr(const char *addr);
	const char *getFidAddr();
	void setFidPort(uint16_t port);
	uint16_t getFidPort();
	void setFidUsername(const char *username);
	const char *getFidUsername();
	void setFidPassword(const char *password);
	const char *getFidPassword();

	void setPaymentBus(uint16_t paymentBus);
	uint16_t getPaymentBus() const;
	void setEvadts(bool value);
	bool getEvadts() const;
	void setCurrency(uint16_t currency);
	uint16_t getCurrency() const;
	void setDecimalPoint(uint16_t decimalPoint);
	uint16_t getDecimalPoint() const;
	void setMaxCredit(uint32_t maxCredit);
	uint32_t getMaxCredit() const;
	void setCashlessNumber(uint8_t cashlessNum);
	uint8_t getCashlessNumber();
	void setCashlessMaxLevel(uint8_t cashlessMaxLevel);
	uint8_t getCashlessMaxLevel();
	void setScaleFactor(uint32_t scaleFactor);
	uint8_t getScaleFactor();
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
	Mdb::DeviceContext *getRemoteCashlessContext();
	Fiscal::Context *getFiscalContext();
	Mdb::DeviceContext *getScreenContext();
	Mdb::DeviceContext *getNfcContext();
	const char *getCommunictionId();
	Config3DeviceList *getDeviceList();

	bool addProduct(const char *selectId, uint16_t cashlessId);
	void addPriceList(const char *device, uint8_t number, Config3PriceIndexType type);
	Config3ProductIterator *createIterator();
	MemoryResult sale(Fiscal::Sale *sale);
	MemoryResult sale(const char *selectId, const char *device, uint16_t number, uint32_t value);
	MemoryResult sale(uint16_t selectId, const char *device, uint16_t number, uint32_t value);
	MemoryResult registerCoinIn(uint32_t , uint8_t ) { return MemoryResult_Ok; }
	MemoryResult registerCoinDispense(uint32_t ) { return MemoryResult_Ok; }
	MemoryResult registerCoinDispenseManually(uint32_t ) { return MemoryResult_Ok; }
	MemoryResult registerCoinFill(uint32_t ) { return MemoryResult_Ok; }
	MemoryResult registerBillIn(uint32_t ) { return MemoryResult_Ok; }
	MemoryResult registerCashOverpay(uint32_t ) { return MemoryResult_Ok; }

	uint32_t getTotalCount() { return products.getTotalCount(); }
	uint32_t getTotalMoney() { return products.getTotalMoney(); }
	uint32_t getCount() { return products.getCount(); }
	uint32_t getMoney() { return products.getMoney(); }
	uint16_t getProductNum() { return productIndexes.getSize(); }
	uint16_t getPriceListNum() { return priceIndexes.getSize(); }
	Config3ProductIndexList *getProductIndexList() { return &productIndexes; }
	Config3PriceIndexList *getPriceIndexList() { return &priceIndexes; }
	Config3ProductList *getProductList() { return &products; }
	Config3BarcodeList *getBarcodeList() { return &barcodes; }
	Config3::WareGroupList *getWareGroupList() { return &wareGroups; }
	Config3::WareLinkList *getWareLinkList() { return &wareLinks; }

private:
	Memory *memory;
	uint32_t address;
	uint32_t automatId;
	uint32_t configId;
	uint16_t taxSystem;	

	uint8_t internetDevice;
	uint8_t ext1Device;
	uint8_t ext2Device;
	uint8_t usb1Device;
	uint8_t qrType;
	uint8_t fiscalPrinter;

	uint16_t paymentBus;
	uint32_t maxCredit;
	uint8_t flags;
	uint8_t cashlessNum;
	uint8_t cashlessMaxLevel;

	Mdb::DeviceContext smContext;
	Mdb::DeviceContext cl3Context;
	Mdb::DeviceContext cl4Context;
	Mdb::DeviceContext remoteCashlessContext;
	Fiscal::Context fiscalContext;
	Mdb::DeviceContext screenContext;
	Mdb::DeviceContext nfcContext;

	Config3PriceIndexList priceIndexes;
	Config3ProductIndexList productIndexes;
	Config3ProductList products;
	Config3DeviceList devices;
	Config3AutomatSettings settings;
	Config3BarcodeList barcodes;
	Config3::WareGroupList wareGroups;
	Config3::WareLinkList wareLinks;

	MemoryResult initData(Memory *memory);
	MemoryResult loadData(Memory *memory);
	MemoryResult saveData();
	static uint32_t calcDevider(uint16_t decimalPoint);
};

#endif
