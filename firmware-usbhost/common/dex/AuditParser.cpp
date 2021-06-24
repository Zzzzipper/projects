#include "dex/include/AuditParser.h"

#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

#define EVADTS_LINE_MAX_SIZE 128
#define EVADTS_TOKEN_MAX_NUM 16
#define SYMBOL_CR 0x0D
#define SYMBOL_LF 0x0A

AuditParser::AuditParser() : audit(NULL) {
	this->str = new char[EVADTS_LINE_MAX_SIZE]();
	this->tokens = new char*[EVADTS_TOKEN_MAX_NUM]();
}

AuditParser::~AuditParser() {
	delete this->str;
	delete this->tokens;
}

void AuditParser::start(Audit *audit) {
	LOG_WARN(LOG_EVADTS, "start");
	this->audit = audit;
	this->audit->clear();
	this->strLen = 0;
	this->symbolCR = false;
	this->product = NULL;
}

bool AuditParser::parse(const uint8_t *data, const uint16_t len) {
	LOG_WARN(LOG_EVADTS, "parse: dataLen=" << len);
	if(audit == NULL) {
		LOG_ERROR(LOG_EVADTS, "Fatal error: start before parse");
		return false;
	}

	for(uint16_t i = 0; i < len; i++) {
		if(data[i] == SYMBOL_CR) {
			symbolCR = true;
		} else if(data[i] == SYMBOL_LF) {
			if(symbolCR == false) { return false; }
			if(addSymbol('\0') == false) { return false; }
			if(procLine() == false) { return false; }
			strLen = 0;
			symbolCR = false;
		} else {
			if(addSymbol(data[i]) == false) { return false; }
			symbolCR = false;
		}
	}

	return true;
}

bool AuditParser::addSymbol(char symbol) {
	if(strLen >= EVADTS_LINE_MAX_SIZE) { return false; }
	str[strLen] = symbol;
	strLen++;
	return true;
}

bool AuditParser::addToken(char *str) {
	if(tokenNum >= EVADTS_TOKEN_MAX_NUM) { return false; }
	tokens[tokenNum] = str;
	tokenNum++;
	LOG_TRACE(LOG_EVADTS, "Token[" << tokenNum << "]" << str);
	return true;
}

bool AuditParser::parseTokens() {
	tokenNum = 0;
	addToken(str);
	uint16_t i = 0;
	while(str[i] != '\0') {
		if(str[i] == '*') {
			str[i] = '\0';
			i++;
			if(addToken(str + i) == false) { return false; }
		} else {
			i++;
		}
	}
	return true;
}

bool AuditParser::procLine() {
	if(parseTokens() == false) {
		LOG_ERROR(LOG_EVADTS, "Too much tokens");
		return false;
	}
	if(strcmp("ID1", tokens[0]) == 0) { return parseID1(); }
	else if(strcmp("ID4", tokens[0]) == 0) { return parseID4(); }
	else if(strcmp("LA1", tokens[0]) == 0) { return parseLA1(); }
	else if(strcmp("PA1", tokens[0]) == 0) { return parsePA1(); }
	else if(strcmp("PA2", tokens[0]) == 0) { return parsePA2(); }
	else if(strcmp("PA7", tokens[0]) == 0) { return parsePA7(); }
	else { LOG_DEBUG(LOG_EVADTS, "Ignore line " << tokens[0]); }
	return true;
}

bool AuditParser::parseID1() {
	enum {
		ID1_AutomatId = 6
	};
	if(tokenNum < (ID1_AutomatId + 1)) {
		LOG_WARN(LOG_EVADTS, "ID1 too less params");
		return true;
	}
	audit->setAutomatId(Sambery::stringToNumber<uint32_t>(tokens[ID1_AutomatId]));
	LOG_INFO(LOG_EVADTS, "Set automatId " << tokens[ID1_AutomatId]);
	return true;
}

bool AuditParser::parseID4() {
	enum {
		ID4_DecimalPoint = 1
	};
	if(tokenNum < (ID4_DecimalPoint + 1)) {
		LOG_WARN(LOG_EVADTS, "ID4 too less params");
		return true;
	}
	audit->setDecimalPlaces(Sambery::stringToNumber<uint16_t>(tokens[ID4_DecimalPoint]));
	LOG_INFO(LOG_EVADTS, "Set decimalPoint " << tokens[ID4_DecimalPoint]);
	return true;
}

//todo: разобраться в приоритетах цен
bool AuditParser::parseLA1() {
	LOG_DEBUG(LOG_EVADTS, "Found LA1");
	enum {
		LA1_PricelistNumber = 1,
		LA1_ProductID = 2,
		LA1_Price = 3,
		LA1_NumberOfVendsSinceLastReset = 4,
		LA1_NumberOfVendsSinceInitialisation = 5,
	};
	if(tokenNum < (LA1_NumberOfVendsSinceInitialisation + 1)) {
		LOG_WARN(LOG_EVADTS, "Ignore LA1");
		return false;
	}

	const char *productId = tokens[LA1_ProductID];
	product = audit->getProductById(productId);
	if(product == NULL) {
		LOG_DEBUG(LOG_EVADTS, "Product not found. Add product " << productId);
		product = new AuditProduct;
		product->id = productId;
		audit->addProduct(product);
	}

	uint32_t productPrice = Sambery::stringToNumber<uint32_t>(tokens[LA1_Price]);
	uint32_t productSaleTotal = Sambery::stringToNumber<uint32_t>(tokens[LA1_NumberOfVendsSinceInitialisation]);
	if(strcmp("0", tokens[LA1_PricelistNumber]) == 0) {
		product->ca0price = productPrice;
		product->ca0saleTotal = productSaleTotal;
		LOG_INFO(LOG_EVADTS, "Product " << productId << " CA0 price=" << productPrice << ", saleTotal=" << productSaleTotal);
	}
	return true;
}

bool AuditParser::parsePA1() {
	LOG_DEBUG(LOG_EVADTS, "Found PA1");
	enum {
		PA1_ProductID = 1,
		PA1_ProductPrice = 2,
		PA1_ProductName = 3,
		PA1_ProductDisable = 7
	};
	if(tokenNum < (PA1_ProductName + 1)) {
		LOG_WARN(LOG_EVADTS, "Ignore PA1");
		return false;
	}

	const char *productId = tokens[PA1_ProductID];
	product = audit->getProductById(productId);
	if(product == NULL) {
		LOG_DEBUG(LOG_EVADTS, "Product not found. Add product " << productId);
		product = new AuditProduct;
		product->id = productId;
		audit->addProduct(product);
	}

	latinToWin1251(tokens[PA1_ProductName]);
	product->name = tokens[PA1_ProductName];
	LOG_INFO(LOG_EVADTS, "Product " << productId << " to " << tokens[PA1_ProductName]);
	return true;
}

bool AuditParser::parsePA2() {
	LOG_DEBUG(LOG_EVADTS, "Found PA2");
	enum {
		PA2_SaleTotal = 1,
		PA2_ValueTotal = 2,
		PA2_SaleSinceReset = 3,
		PA2_ValueSinceReset = 4,
	};
	if(tokenNum < PA2_SaleSinceReset) {
		LOG_ERROR(LOG_EVADTS, "Ignore PA2");
		return false;
	}

	if(product == NULL) {
		LOG_ERROR(LOG_EVADTS, "Alone tag PA2");
		return false;
	}

	uint32_t productSaleTotal = Sambery::stringToNumber<uint32_t>(tokens[PA2_SaleTotal]);
	product->ca0saleTotal = productSaleTotal;
	LOG_INFO(LOG_EVADTS, "Product " << product->id.getString() << " CA0 saleTotal=" << productSaleTotal);
	return true;
}

bool AuditParser::parsePA7() {
	LOG_DEBUG(LOG_EVADTS, "Found PA7");
	enum {
		PA7_ProductID = 1,
		PA7_PaymentDevice = 2,
		PA7_PriceListNumber = 3,
		PA7_Price = 4,
		PA7_SaleTotal = 5,
		PA7_ValueTotal = 6,
		PA7_Sale = 7,
		PA7_Value = 8
	};
	if(tokenNum < PA7_ValueTotal) {
		LOG_ERROR(LOG_EVADTS, "Ignore PA7");
		return false;
	}

	const char *productId = tokens[PA7_ProductID];
	AuditProduct *product = audit->getProductById(productId);
	if(product == NULL) {
		LOG_ERROR(LOG_EVADTS, "Product " << productId << " not found");
		return false;
	}

	uint32_t productPrice = Sambery::stringToNumber<uint32_t>(tokens[PA7_Price]);
	uint32_t productSaleTotal = Sambery::stringToNumber<uint32_t>(tokens[PA7_SaleTotal]);
	if(strcmp("CA", tokens[PA7_PaymentDevice]) == 0 && strcmp("0", tokens[PA7_PriceListNumber]) == 0) {
		product->ca0price = productPrice;
		product->ca0saleTotal = productSaleTotal;
		LOG_INFO(LOG_EVADTS, "Product " << productId << " CA0 price=" << productPrice << ", saleTotal=" << productSaleTotal);
	}
	return true;
}

uint8_t AuditParser::charToValue(char lex) {
	char valueHigh[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	char valueLow[]  = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	for(uint8_t i = 0; i < sizeof(valueHigh); i++) {
		if(lex == valueHigh[i]) {
			return i;
		}
	}
	for(uint8_t i = 0; i < sizeof(valueLow); i++) {
		if(lex == valueLow[i]) {
			return i;
		}
	}
	return 255;
}

bool AuditParser::latinToWin1251(char *name) {
	char *from = name;
	char *to = name;
	uint8_t hex;
	char symbol;
	while(*from != '\0') {
		if(*from == '^') {
			from++;
			if(*from == '\0') {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			hex = charToValue(*from);
			if(hex > 15) {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			symbol = (hex << 4);

			from++;
			if(*from == '\0') {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			hex = charToValue(*from);
			if(hex > 15) {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
			symbol |= hex;

			from++;
			if(*from != '^') {
				LOG_ERROR(LOG_EVADTS, "Wrong name format");
				*to = '\0';
				return false;
			}
 		} else {
			symbol = *from;
		}
		*to = symbol;
		to++;
		from++;
	}
	*to = '\0';
	return true;
}
