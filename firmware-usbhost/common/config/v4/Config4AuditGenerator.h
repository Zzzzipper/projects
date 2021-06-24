#ifndef COMMON_CONFIG_V4_AUDITGENERATOR_H_
#define COMMON_CONFIG_V4_AUDITGENERATOR_H_

#include "Config4Modem.h"
#include "evadts/EvadtsGenerator.h"

class Config4AuditGenerator : public EvadtsGenerator {
public:
	Config4AuditGenerator(Config4Modem *config);
	virtual ~Config4AuditGenerator();
	virtual void reset();
	virtual void next();
	virtual bool isLast();

private:
	enum State {
		State_Header = 0,
		State_Main,
		State_CoinChanger,
		State_Products,
		State_ProductPrices,
		State_Footer,
		State_Complete,
	};

	Config4Modem *config;
	State state;
	Config4ProductIterator *product;
	uint16_t priceListIndex;

	void generateMain();
	void generateCoinChanger();
	void generateProducts();
	void generateProductPrices();
};

#endif
