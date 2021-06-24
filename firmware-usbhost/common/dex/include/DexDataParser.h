#ifndef DEX_DATAPARSER_H_
#define DEX_DATAPARSER_H_

#include "utils/include/Event.h"

namespace Dex {

class DataParser {
public:
	enum EventType {
		Event_AsyncOk	 = GlobalId_DataParser | 0x01,
		Event_AsyncError = GlobalId_DataParser | 0x02,
	};

	enum Result {
		Result_Ok = 0,
		Result_Async,
		Result_Busy,
		Result_Error
	};

	virtual ~DataParser() {}
	virtual void setObserver(EventObserver *) {}
	virtual Result start(uint32_t size) = 0;
	virtual Result procData(const uint8_t *data, const uint16_t len) = 0;
	virtual Result complete() = 0;
	virtual void error() = 0;
};

}

#endif
