#ifndef COMMON_CONFIG_V4_MEIAUDITGENERATOR_H_
#define COMMON_CONFIG_V4_MEIAUDITGENERATOR_H_

#include "Config4Modem.h"
#include "evadts/EvadtsGenerator.h"

class Config4MeiAuditGenerator : public EvadtsGenerator {
public:
	Config4MeiAuditGenerator(Config4Modem *config);
	virtual ~Config4MeiAuditGenerator();
	virtual void reset();
	virtual void next();
	virtual bool isLast();

private:
	enum State {
		State_Header = 0,
		State_Main,
		State_CoinChanger,
		State_PriceList0,
		State_PriceList1,
		State_Products,
		State_Total,
		State_Footer,
		State_Complete,
	};

	Config4Modem *config;
	State state;
	Config4ProductIterator *product;

	void generateCoinChanger();
	void generateMain();
	void gotoStatePriceList0();
	void generatePriceList0();
	void gotoStatePriceList1();
	void generatePriceList1();
	void gotoStateProducts();
	void generateProducts();
	void gotoStateTotal();
	void generateTotal();
};

#endif
