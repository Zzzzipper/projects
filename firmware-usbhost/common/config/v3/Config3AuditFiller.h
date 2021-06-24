#ifndef COMMON_CONFIG_V3_AUDITFILLER_H_
#define COMMON_CONFIG_V3_AUDITFILLER_H_

#include "automat/Config3Automat.h"
#include "evadts/EvadtsParser.h"

class FlagContainer {
public:
	FlagContainer(uint16_t indexMax);
	~FlagContainer();
	void clear();
	bool isExist(uint16_t index);
	void setFlag(uint16_t index);
	bool getFlag(uint16_t index);

private:
	uint16_t indexMax;
	uint8_t *flag;
};

class Config3AuditFiller : public Evadts::Parser {
public:
	Config3AuditFiller(Config3Automat *config);
	virtual ~Config3AuditFiller();

private:
	Config3Automat *config;
	Config3ProductIterator *product;
	FlagContainer flags;

	virtual void procStart();
	virtual bool procLine(char **tokens, uint16_t tokenNum);
	virtual void procComplete();
	bool parseID4(char **tokens, uint16_t tokenNum);
	bool parseCA15(char **tokens, uint16_t tokenNum);
	bool parseCA17(char **tokens, uint16_t tokenNum);
	bool parsePA1(char **tokens, uint16_t tokenNum);
	bool parsePA2(char **tokens, uint16_t tokenNum);
	bool parsePA7(char **tokens, uint16_t tokenNum);
	bool parseLA1(char **tokens, uint16_t tokenNum);
};

#endif
