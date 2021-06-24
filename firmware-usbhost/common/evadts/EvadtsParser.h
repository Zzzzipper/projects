#ifndef COMMON_DEX_EVADTSPARSER_H_
#define COMMON_DEX_EVADTSPARSER_H_

#include <stdint.h>

namespace Evadts {

class Parser {
public:
	Parser();
	virtual ~Parser();
	void start();
	void procData(const uint8_t *data, const uint16_t len);
	void complete();
	bool hasError() { return error; }

protected:
	virtual void procStart() = 0;
	virtual bool procLine(char **tokens, uint16_t tokenNum) = 0;
	virtual void procComplete() = 0;

	bool hasToken(uint16_t index);
	bool hasTokenValue(uint16_t index);

private:
	char *str;
	uint16_t strLen;
	bool symbolCR;
	char **tokens;
	uint16_t tokenNum;
	bool error;

	bool addSymbol(char symbol);
	bool addToken(char *str);
	bool parseTokens();
};

extern bool latinToWin1251(char *name);

}

#endif
