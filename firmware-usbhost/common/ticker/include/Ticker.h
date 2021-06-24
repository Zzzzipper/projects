#ifndef TICKER_H_
#define TICKER_H_

#include "TickerListener.h"

class Ticker {
public:
    static Ticker *get();
	void tick();
	void registerConsumer(TickerListener *consumer);

private:
	TickerListener *consumer;

	Ticker();
	~Ticker();
	Ticker( const Ticker &c );
	Ticker& operator=( const Ticker &c );
};

#endif
