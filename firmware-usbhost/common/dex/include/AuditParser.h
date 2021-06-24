#ifndef AUDIT_PARSER
#define AUDIT_PARSER

#include "Audit.h"

class AuditParser {
public:
	AuditParser();
	virtual ~AuditParser();
	void start(Audit *audit);
	bool parse(const uint8_t *data, const uint16_t len);
	static uint8_t charToValue(char lex);
	static bool latinToWin1251(char *name);

private:
	Audit *audit;
	char *str;
	uint16_t strLen;
	bool symbolCR;
	char **tokens;
	uint16_t tokenNum;
	AuditProduct *product;

	bool addSymbol(char symbol);
	bool addToken(char *str);
	bool parseTokens();
	bool procLine();
	bool parseID1();
	bool parseID4();
	bool parseLA1();
	bool parsePA1();
	bool parsePA2();
	bool parsePA7();
};

#endif // AUDIT_PARSER
