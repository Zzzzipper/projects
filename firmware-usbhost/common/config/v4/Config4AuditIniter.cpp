#include "config/v4/Config4AuditIniter.h"
#include "config/v4/Config4Modem.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

Config4AuditIniter::Config4AuditIniter(Config4Modem *config) :
	Evadts::Parser(),
	config(config) {
}

void Config4AuditIniter::procStart() {
	LOG_DEBUG(LOG_EVADTS, "procStart");
	config->getAutomat()->shutdown();
}

void Config4AuditIniter::procComplete() {
	LOG_DEBUG(LOG_EVADTS, "complete");
	config->init();
}

bool Config4AuditIniter::procLine(char **tokens, uint16_t tokenNum) {
	if(strcmp("ID4", tokens[0]) == 0) { return parseID4(tokens, tokenNum); }
	else if(strcmp("PA1", tokens[0]) == 0) { return parsePA1(tokens, tokenNum); }
	else if(strcmp("PA7", tokens[0]) == 0) { return parsePA7(tokens, tokenNum); }
	else { LOG_DEBUG(LOG_EVADTS, "Ignore line " << tokens[0]); }
	return true;
}

bool Config4AuditIniter::parseID4(char **tokens, uint16_t tokenNum) {
	enum {
		ID4_DecimalPoint = 1
	};
	if(tokenNum < (ID4_DecimalPoint + 1)) {
		LOG_WARN(LOG_EVADTS, "ID4 too less params");
		return true;
	}
	config->getAutomat()->setDecimalPoint(Sambery::stringToNumber<uint16_t>(tokens[ID4_DecimalPoint]));
	LOG_INFO(LOG_EVADTS, "Set decimalPoint " << tokens[ID4_DecimalPoint]);
	return true;
}

/**
 * NOTE: CashlessId невозможно получить из аудита, поэтому он устанавливается от балды.
 */
bool Config4AuditIniter::parsePA1(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parsePA1");
	enum {
		PA1_ProductID = 1,
		PA1_ProductPrice = 2,
		PA1_ProductName = 3,
		PA1_ProductDisable = 7
	};
	if(tokenNum < (PA1_ProductID + 1)) {
		LOG_WARN(LOG_EVADTS, "Wrong PA1: too few parameters");
		return false;
	}

	const char *productId = tokens[PA1_ProductID];
	config->getAutomat()->addProduct(productId, Config3ProductIndexList::UndefinedIndex);
	LOG_INFO(LOG_EVADTS, "Add product " << productId);
	return true;
}

bool Config4AuditIniter::parsePA7(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parsePA7");
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
	if(tokenNum < (PA7_PriceListNumber + 1)) {
		LOG_ERROR(LOG_EVADTS, "Ignore PA7");
		return false;
	}

	uint16_t priceListNum = Sambery::stringToNumber<uint16_t>(tokens[PA7_PriceListNumber]);
	config->getAutomat()->addPriceList(tokens[PA7_PaymentDevice], priceListNum, Config3PriceIndexType_None);
	LOG_INFO(LOG_EVADTS, "Add price list " << tokens[PA7_PaymentDevice] << priceListNum);
	return true;
}
