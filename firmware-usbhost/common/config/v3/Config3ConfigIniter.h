#ifndef COMMON_CONFIG_V3_CONFIGINITER_H_
#define COMMON_CONFIG_V3_CONFIGINITER_H_

#include "config/v3/automat/Config3Automat.h"
#include "evadts/EvadtsParser.h"

class Config3ConfigIniter : public Evadts::Parser {
public:
	Config3ConfigIniter();
	Config3PriceIndexList *getPrices() { return &prices; }
	Config3ProductIndexList *getProducts() { return &products; }

private:
	uint16_t paymentBus;
	Config3PriceIndexList prices;
	Config3ProductIndexList products;

	virtual void procStart();
	virtual bool procLine(char **tokens, uint16_t tokenNum);
	virtual void procComplete();
	bool parseAC2(char **tokens, uint16_t tokenNum);
	bool parseIC4(char **tokens, uint16_t tokenNum);
	bool parsePC1(char **tokens, uint16_t tokenNum);
	bool parsePC7(char **tokens, uint16_t tokenNum);
	bool parsePC9(char **tokens, uint16_t tokenNum);
};

#endif
