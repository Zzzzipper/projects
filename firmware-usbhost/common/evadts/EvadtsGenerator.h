#ifndef COMMON_DEX_EVADTSGENERATOR_H_
#define COMMON_DEX_EVADTSGENERATOR_H_

#include "dex/include/DexDataGenerator.h"
#include "dex/DexCrc.h"

class StringBuilder;

class EvadtsGenerator : public Dex::DataGenerator {
public:
	EvadtsGenerator(const char *communicactionId);
	virtual ~EvadtsGenerator();
	virtual void reset() = 0;
	virtual void next() = 0;
	virtual bool isLast() = 0;
	const void *getData();
	uint16_t getLen();

protected:
	StringBuilder *str;
	void startBlock();
	void win1251ToLatin(const char *s);
	void finishLine();
	void finishBlock();
	void generateHeader();
	void generateFooter();

private:
	const char *communicactionId;
	uint16_t strCount;
	Dex::Crc crc;
};

#endif
