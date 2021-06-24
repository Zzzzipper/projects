#ifndef DEX_TEST_DATAPARSER_H
#define DEX_TEST_DATAPARSER_H

#include "dex/include/DexDataParser.h"
#include "utils/include/StringBuilder.h"

class TestDataParser : public Dex::DataParser {
public:
	TestDataParser();
	TestDataParser(uint16_t maxSize, bool hex);
	virtual ~TestDataParser();
	void setStartResult(Result result);
	void setProcDataResult(Result result);
	void setCompleteResult(Result result);
	void clearData();
	const char *getData();

	virtual Result start(uint32_t dataSize);
	virtual Result procData(const uint8_t *data, const uint16_t dataLen);
	virtual Result complete();
	virtual void error();

private:
	bool hex;
	StringBuilder data;
	Result startResult;
	Result procDataResult;
	Result completeResult;
};

#endif
