#ifndef COMMON_CONFIG_V3_AUDITGENERATOR_H_
#define COMMON_CONFIG_V3_AUDITGENERATOR_H_

#include "Config3Modem.h"
#include "evadts/EvadtsGenerator.h"

class StringBuilder;

class Config3AuditGenerator : public EvadtsGenerator {
public:
	Config3AuditGenerator(Config3Modem *config);
	virtual ~Config3AuditGenerator();
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

	Config3Modem *config;
	State state;
	Config3ProductIterator *product;
	uint16_t priceListIndex;

	void generateMain();
	void generateCoinChanger();
	void generateProducts();
	void generateProductPrices();
};

#endif
