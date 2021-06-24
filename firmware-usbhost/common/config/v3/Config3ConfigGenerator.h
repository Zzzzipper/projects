#ifndef CONFIG_CONFIG_V3_CONFIGGENERATOR_H_
#define CONFIG_CONFIG_V3_CONFIGGENERATOR_H_

#include "Config3Modem.h"
#include "evadts/EvadtsGenerator.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/StringParser.h"

class Config3ConfigGenerator : public EvadtsGenerator {
public:
	Config3ConfigGenerator(Config3Modem *config);
	virtual ~Config3ConfigGenerator();
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
		State_Custom,
		State_Footer,
		State_Complete,
	};

	Config3Modem *config;
	State state;
	StringBuilder cert;
	StringParser parser;
	uint16_t certOffset;
	Config3ProductIterator *product;

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
	void generateCustom();
};

#endif
