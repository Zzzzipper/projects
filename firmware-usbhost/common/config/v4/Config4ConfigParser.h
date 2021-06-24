#ifndef COMMON_CONFIG_V4_CONFIGPARSER_H_
#define COMMON_CONFIG_V4_CONFIGPARSER_H_

#include "Config4Modem.h"
#include "evadts/EvadtsParser.h"

class Config4ConfigParser : public Evadts::Parser {
public:
	Config4ConfigParser(Config4Modem *config, bool fixedDecimalPoint = false);
	virtual ~Config4ConfigParser();
	void setFixedDecimalPoint(bool fixedDecimalPoint);

private:
	Config4Modem *config;
	Config4ProductIterator *product;
	bool fixedDecimalPoint;
	uint32_t devider;
	StringBuilder cert;

	virtual void procStart();
	virtual bool procLine(char **tokens, uint16_t tokenNum);
	virtual void procComplete();
	bool parseAC2(char **tokens);
	bool parseFC1(char **tokens);
	bool parseFC2(char **tokens);
	bool parseFC3(char **tokens);
	bool parseFC4(char **tokens);
	bool parseFC5(char **tokens);
	bool parseIC1(char **tokens);
	bool parseIC4(char **tokens);
	bool parseLC2(char **tokens);
	bool parsePC1(char **tokens);
	bool parsePC7(char **tokens);
	bool parsePC9(char **tokens, uint16_t tokenNum);
	bool setDevider(uint32_t decimalPoint);
	uint32_t convertValue(uint32_t value);
};

#endif
