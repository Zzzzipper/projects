#ifndef COMMON_TICKER_LISTENER_H_
#define COMMON_TICKER_LISTENER_H_

#include <stdint.h>

class TickerListener {
public:
	virtual void tick(uint32_t tickSize) = 0;
};

#endif
