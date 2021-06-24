#ifndef COMMON_CONFIG_V3_CONFIGPARSER_H_
#define COMMON_CONFIG_V3_CONFIGPARSER_H_

#include "Config3Modem.h"
#include "evadts/EvadtsParser.h"

class Config3ConfigParser : public Evadts::Parser {
public:
	Config3ConfigParser(Config3Modem *config, bool fixedDecimalPoint = false);
	virtual ~Config3ConfigParser();
	void setFixedDecimalPoint(bool fixedDecimalPoint);

private:
	Config3Modem *config;
	Config3ProductIterator *product;
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
	bool parseMC5(char **tokens);
	bool parseMC5_INTERNET(char **tokens);
	bool parseMC5_EXT1(char **tokens);
	bool parseMC5_EXT2(char **tokens);
	bool parseMC5_USB1(char **tokens);
	bool parseMC5_QRTYPE(char **tokens);
	bool parseMC5_ETH1MAC(char **tokens);
	bool parseMC5_ETH1ADDR(char **tokens);
	bool parseMC5_ETH1MASK(char **tokens);
	bool parseMC5_ETH1GW(char **tokens);
	bool parseMC5_FIDTYPE(char **tokens);
	bool parseMC5_FIDADDR(char **tokens);
	bool parseMC5_FIDDEVICE(char **tokens);
	bool parseMC5_FIDAUTH(char **tokens);
	bool parseMC5_BC(char **tokens);
	bool parseMC5_BV(char **tokens);
	bool parseMC5_WG(char **tokens);
	bool parseMC5_WL(char **tokens);
	bool setDevider(uint32_t decimalPoint);
	uint32_t convertValue(uint32_t value);
};

#endif
