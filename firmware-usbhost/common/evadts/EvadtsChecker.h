#ifndef COMMON_DEX_EVADTSCHECKER_H_
#define COMMON_DEX_EVADTSCHECKER_H_

#include "evadts/EvadtsParser.h"
#include "dex/DexCrc.h"

namespace Evadts {

class Checker {
public:
	Checker();
	virtual ~Checker();
	void start();
	void procData(const uint8_t *data, const uint16_t len);
	void complete();
	bool hasError();

private:
	char *str;
	uint16_t strLen;
	bool symbolCR;
	Dex::Crc crc;
	bool error;

	bool addSymbol(char symbol);
	bool procLine();
	bool procG85();
	bool procOther();
};

}

#endif
