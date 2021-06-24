#ifndef COMMON_CONFIG_V4_AUDITINITER_H_
#define COMMON_CONFIG_V4_AUDITINITER_H_

#include "evadts/EvadtsParser.h"

class Config4Modem;

class Config4AuditIniter : public Evadts::Parser {
public:
	Config4AuditIniter(Config4Modem *config);

private:
	Config4Modem *config;

	virtual void procStart();
	virtual bool procLine(char **tokens, uint16_t tokenNum);
	virtual void procComplete();
	bool parseID4(char **tokens, uint16_t tokenNum);
	bool parsePA1(char **tokens, uint16_t tokenNum);
	bool parsePA7(char **tokens, uint16_t tokenNum);
};

#endif
