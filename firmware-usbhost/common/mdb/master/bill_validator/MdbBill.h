#ifndef MDB_BILL_H_
#define MDB_BILL_H_

#include "mdb/MdbProtocol.h"
#include "timer/include/DateTime.h"
#include "timer/include/RealTime.h"

#include <stdint.h>

#define BV_STACKER_UNKNOWN 0xFFFF

class MdbBill {
public:
	void setNominal(uint32_t nominal);
	uint32_t getNominal();

private:
	uint32_t nominal;
};

class MdbBillValidatorContext : public Mdb::DeviceContext {
public:
	MdbBillValidatorContext(uint32_t masterDecimalPoint, RealTimeInterface *realtime, uint32_t maxCredit);
	~MdbBillValidatorContext();
	void init(uint8_t level, uint32_t deviceDecimalPlaces, uint32_t scalingFactor, uint8_t *data, uint8_t num);
	void resetChanged();
	bool getChanged();
	uint16_t getMask();
	uint32_t getBillNominal(uint8_t index);
	uint8_t getBillValue(uint8_t index);
	uint8_t getBillNumber();
	uint8_t getLevel();
	void setUseChange(bool value);
	bool getUseChange();
	void setMaxChange(uint32_t maxChange);
	void setMaxBill(uint32_t maxBill);
	uint32_t getMaxBill();
	void setBillInStacker(uint16_t billInStacker);
	uint16_t getBillInStacker();
	void registerLastBill(uint32_t value);
	DateTime *getLastBillDatetime();
	uint32_t getLastBillValue();

private:
	RealTimeInterface *realtime;
	bool changed;
	uint8_t level;
	MdbBill *bills;
	bool useChange;
	uint32_t maxChange;
	uint32_t maxBill;
	uint16_t billInStacker;
	DateTime lastBillDatetime;
	uint32_t lastBillValue;
};

#endif
