#ifndef DEX_TEST_DATAGENERATOR_H
#define DEX_TEST_DATAGENERATOR_H

#include "dex/include/DexDataGenerator.h"
#include "utils/include/StringBuilder.h"

class TestDataGenerator : public Dex::DataGenerator {
public:
	TestDataGenerator();
	virtual ~TestDataGenerator();
	virtual void reset();
	virtual void next();
	virtual bool isLast();
	virtual const void *getData();
	virtual uint16_t getLen();

private:
	enum State {
		State_Idle = 0,
		State_Data0,
		State_Data1,
		State_Data2
	};
	State state;
	StringBuilder data;
};

#endif
