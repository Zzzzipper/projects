#ifndef COMMON_CONFIG_CONFIGAUDITINITER_H_
#define COMMON_CONFIG_CONFIGAUDITINITER_H_

#include "Config3Modem.h"
#include "evadts/EvadtsParser.h"

class Config3AuditIniter : public Evadts::Parser {
public:
	Config3AuditIniter(Config3Modem *config);

private:
	Config3Modem *config;

	virtual void procStart();
	virtual bool procLine(char **tokens, uint16_t tokenNum);
	virtual void procComplete();
	bool parseID4(char **tokens, uint16_t tokenNum);
	bool parsePA1(char **tokens, uint16_t tokenNum);
	bool parsePA7(char **tokens, uint16_t tokenNum);
};

#endif
