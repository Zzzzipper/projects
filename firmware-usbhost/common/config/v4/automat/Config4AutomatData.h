#ifndef COMMON_CONFIG_V4_AUTOMAT_DATA_H_
#define COMMON_CONFIG_V4_AUTOMAT_DATA_H_

#include "mdb/MdbProtocol.h"
#include "memory/include/Memory.h"

class Config4AutomatData {
public:
	Config4AutomatData(RealTimeInterface *realtime);
	void setDefault();
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	Mdb::DeviceContext *getContext();
	void setAutomatId(uint32_t automatId);
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
	uint8_t getExt1Device() const;
	void setCashlessNumber(uint8_t cashlessNum);
	uint8_t getCashlessNumber() const;
	void setCashlessMaxLevel(uint8_t cashlessMaxLevel);
	uint8_t getCashlessMaxLevel();
	void setScaleFactor(uint32_t scaleFactor);
	uint8_t getScaleFactor() const;
	void setQrCode(uint32_t qrCode);
	uint8_t getQrCode();
	void setPriceHolding(bool priceHolding);
	bool getPriceHolding() const;
	void setCategoryMoney(bool categoryMoney);
	bool getCategoryMoney() const;
	void setShowChange(bool showChange);
	bool getShowChange() const;
	void setCreditHolding(bool creditHolding);
	bool getCreditHolding() const;
	void setMultiVend(bool multiVend);
	bool getMultiVend() const;
	void setCashless2Click(bool cashless2Click);
	bool getCashless2Click();

private:
	Memory *memory;
	Mdb::DeviceContext smContext;
	uint32_t address;
	uint32_t automatId;
	uint32_t configId;
	uint16_t paymentBus;
	uint16_t taxSystem;
	uint32_t maxCredit;
	uint8_t ext1Device;
	uint8_t flags;
	uint8_t cashlessNum;
	uint8_t cashlessMaxLevel;
	uint8_t qrCode;
};

#endif
