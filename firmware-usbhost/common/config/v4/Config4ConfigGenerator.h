#ifndef COMMON_CONFIG_V4_CONFIGGENERATOR_H_
#define COMMON_CONFIG_V4_CONFIGGENERATOR_H_

#include "Config4Modem.h"
#include "evadts/EvadtsGenerator.h"
#include "utils/include/StringParser.h"

class Config4ConfigGenerator : public EvadtsGenerator {
public:
	Config4ConfigGenerator(Config4Modem *config);
	virtual ~Config4ConfigGenerator();
	virtual void reset();
	virtual void next();
	virtual bool isLast();

private:
	enum State {
		State_Header = 0,
		State_Main,
		State_FiscalSection,
		State_AuthPublicKey,
		State_AuthPrivateKey,
		State_SignPrivateKey,
		State_PriceLists,
		State_Products,
		State_Footer,
		State_Complete,
	};

	Config4Modem *config;
	State state;
	StringBuilder cert;
	StringParser parser;
	uint16_t certOffset;
	Config4ProductIterator *product;

	void generateMain();
	void generateFiscalSection();
	void gotoStateAuthPublicKey();
	void generateAuthPublicKey();
	void gotoStateAuthPrivateKey();
	void generateAuthPrivateKey();
	void gotoStateSignPrivateKey();
	void generateSignPrivateKey();
	void gotoStateGeneratePriceLists();
	void generatePriceLists();
	void generateProducts();
	void generateProductPrices();
};

#endif
