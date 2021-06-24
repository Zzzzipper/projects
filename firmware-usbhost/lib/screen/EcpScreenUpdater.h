#ifndef LIB_SCREEN_UPDATER_H_
#define LIB_SCREEN_UPDATER_H_

#include "Screen.h"

#include "common/dex/include/DexDataParser.h"

class EcpScreenUpdater : public Dex::DataParser {
public:
	EcpScreenUpdater(Screen *screen);
	virtual Result start(uint32_t size);
	virtual Result procData(const uint8_t *data, const uint16_t len);
	virtual Result complete();
	virtual void error();

private:
	Screen *screen;
	uint32_t dataSize;
	uint32_t dataCount;
	bool delay;

	bool getScreenStage(Screen::Reg24 &stage);
};

#endif
