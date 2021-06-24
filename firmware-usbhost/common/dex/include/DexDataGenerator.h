#ifndef DEX_DATAGENERATOR_H_
#define DEX_DATAGENERATOR_H_

#include <stdint.h>

namespace Dex {

class DataGenerator {
public:
	virtual void reset() = 0;
	virtual void next() = 0;
	virtual bool isLast() = 0;
	virtual const void *getData() = 0;
	virtual uint16_t getLen() = 0;
	virtual uint32_t getSize() { return 0; }
	virtual uint32_t getCurrent() { return 0; }
};

class CommandResult {
public:
	virtual void success() = 0;
	virtual void error() = 0;
};

}

#endif
