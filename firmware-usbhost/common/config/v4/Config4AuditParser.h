#ifndef COMMON_CONFIG_V4_AUDITPARSER_H_
#define COMMON_CONFIG_V4_AUDITPARSER_H_

#include "config/v4/Config4Modem.h"
#include "evadts/EvadtsParser.h"

class Config4AuditParser : public Evadts::Parser {
public:
	Config4AuditParser(Config4Modem *config);
	virtual ~Config4AuditParser();

private:
	Config4Modem *config;
	Config4ProductIterator *product;

	virtual void procStart();
	virtual bool procLine(char **tokens, uint16_t tokenNum);
	virtual void procComplete();
	bool parseAM2(char **tokens, uint16_t tokenNum);
	bool parseAM3(char **tokens, uint16_t tokenNum);
	bool parseID1(char **tokens, uint16_t tokenNum);
	bool parseID4(char **tokens, uint16_t tokenNum);
//	bool parseLA1(char **tokens, uint16_t tokenNum);
	bool parsePA1(char **tokens, uint16_t tokenNum);
	bool parsePA2(char **tokens, uint16_t tokenNum);
	bool parsePA4(char **tokens, uint16_t tokenNum);
	bool parsePA7(char **tokens, uint16_t tokenNum);
	bool parsePA9(char **tokens, uint16_t tokenNum);
};

#endif
