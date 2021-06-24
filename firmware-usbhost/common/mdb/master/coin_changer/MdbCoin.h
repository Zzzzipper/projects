#ifndef COMMON_MDB_MASTER_COINCHANGER_COIN_H_
#define COMMON_MDB_MASTER_COINCHANGER_COIN_H_

#include "mdb/MdbProtocol.h"
#include "utils/include/DecimalPoint.h"
#include "timer/include/RealTime.h"

#include <stdint.h>

#define MDB_CC_TOKEN 0xFFFFFFFF

class MdbCoin {
public:
	MdbCoin();
	void setNominal(uint32_t nominal);
	void setInTube(bool value);
	void setFullTube(bool value);
	void setNumber(uint8_t number);
	uint32_t getNominal();
	bool getInTube();
	bool getFullTube();
	uint8_t getNumber();

private:
	uint32_t nominal;
	bool inTube;
	bool fullTube;
	uint8_t number;
};

class MdbCoinChangerContext : public Mdb::DeviceContext {
public:
	MdbCoinChangerContext(uint32_t masterDecimalPoint, RealTimeInterface *realtime);
	~MdbCoinChangerContext();
	void init(uint16_t deviceDecimalPoint, uint16_t scalingFactor); //todo: удалить
	void init(uint8_t level, uint16_t deviceDecimalPoint, uint16_t scalingFactor, uint8_t *data, uint16_t num, uint16_t coinTypeRouting);
	uint32_t value2money(uint32_t value) override;
	uint32_t money2value(uint32_t value) override;
	void resetChanged();
	bool getChanged();
	void update(uint16_t status, uint8_t *data, uint16_t num);
	void updateTube(uint16_t coinIndex, uint8_t coinNumber);
	void setInTubeValue(uint32_t value);
	uint32_t getInTubeValue();
	MdbCoin *get(uint16_t index);
	uint8_t getSize();
	uint16_t getMask();
	uint16_t getFullMask();
	uint16_t getNotFullMask();
	uint16_t getInTubeMask();
	bool hasChange();
	uint8_t getLevel();
	void registerLastCoin(uint32_t value);
	DateTime *getLastCoinDatetime();
	uint32_t getLastCoinValue();

private:
	RealTimeInterface *realtime;
	bool changed;
	uint8_t level;
	MdbCoin *coins;
	uint32_t inTubeValue;
	DateTime lastCoinDatetime;
	uint32_t lastCoinValue;
	uint32_t minimalChange;
};

#endif
