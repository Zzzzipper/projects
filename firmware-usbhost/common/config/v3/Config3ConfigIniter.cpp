#include "Config3ConfigIniter.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

Config3ConfigIniter::Config3ConfigIniter() :
	Evadts::Parser()
{
}

void Config3ConfigIniter::procStart() {
	LOG_DEBUG(LOG_EVADTS, "procStart");
	paymentBus = Config3Automat::PaymentBus_None;
	prices.clear();
	prices.add("CA", 0, Config3PriceIndexType_None);
	prices.add("CA", 1, Config3PriceIndexType_None);
//	prices.add("CA", 2, Config3PriceIndexType_None);
//	prices.add("CA", 3, Config3PriceIndexType_None);
	prices.add("DA", 0, Config3PriceIndexType_None);
	prices.add("DA", 1, Config3PriceIndexType_None);
//	prices.add("DA", 2, Config3PriceIndexType_None);
//	prices.add("DA", 3, Config3PriceIndexType_None);
	products.clear();
}

void Config3ConfigIniter::procComplete() {
	LOG_DEBUG(LOG_EVADTS, "complete");
	for(uint16_t i = 0; i < prices.getSize(); i++) {
		Config3PriceIndex *priceIndex = prices.get(i);
		LOG_INFO(LOG_EVADTS, "Pricelist " << priceIndex->device.get() << priceIndex->number);
	}
}

bool Config3ConfigIniter::procLine(char **tokens, uint16_t tokenNum) {
	if(strcmp("IC4", tokens[0]) == 0) { return parseIC4(tokens, tokenNum); }
	else if(strcmp("AC2", tokens[0]) == 0) { return parseAC2(tokens, tokenNum); }
	else if(strcmp("PC1", tokens[0]) == 0) { return parsePC1(tokens, tokenNum); }
	else if(strcmp("PC7", tokens[0]) == 0) { return parsePC7(tokens, tokenNum); }
	else if(strcmp("PC9", tokens[0]) == 0) { return parsePC9(tokens, tokenNum); }
	else { LOG_DEBUG(LOG_EVADTS, "Ignore line " << tokens[0]); }
	return true;
}

bool Config3ConfigIniter::parseAC2(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parseAC2");
	enum {
		AC2_PaymentBus = 5,
	};
	if(tokenNum >= (AC2_PaymentBus + 1) && tokens[AC2_PaymentBus][0] != '\0') {
		uint16_t paymentBus = Sambery::stringToNumber<uint16_t>(tokens[AC2_PaymentBus]);
		this->paymentBus = paymentBus;
		LOG_INFO(LOG_EVADTS, "PaymentBus to " << paymentBus);
	}
	return true;
}

bool Config3ConfigIniter::parseIC4(char **tokens, uint16_t tokenNum) {
	enum {
		IC4_DecimalPoint = 1
	};
	if(tokenNum < (IC4_DecimalPoint + 1)) {
		LOG_WARN(LOG_EVADTS, "IC4 too less params");
		return true;
	}
//	config->getModem()->setDecimalPoint(Sambery::stringToNumber<uint16_t>(tokens[IC4_DecimalPoint]));
	LOG_INFO(LOG_EVADTS, "Set decimalPoint " << tokens[IC4_DecimalPoint]);
	return true;
}

bool Config3ConfigIniter::parsePC1(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parsePC1");
	enum {
		PC1_ProductID = 1
	};
	if(tokenNum < (PC1_ProductID + 1)) {
		LOG_WARN(LOG_EVADTS, "Wrong PC1: too few parameters.");
		return false;
	}

	const char *productId = tokens[PC1_ProductID];
	if(products.add(productId, Config3ProductIndexList::UndefinedIndex) == false) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " already exist");
		return false;
	}

	LOG_INFO(LOG_EVADTS, "Add product " << productId);
	return true;
}

bool Config3ConfigIniter::parsePC7(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parsePC7");
	enum {
		PC7_ProductID = 1,
		PC7_PaymentDevice = 2,
		PC7_PriceListNumber = 3,
		PC7_Price = 4
	};
	if(tokenNum < (PC7_PriceListNumber + 1)) {
		LOG_ERROR(LOG_EVADTS, "Ignore PC7");
		return false;
	}
#if 0
	uint16_t priceListNum = Sambery::stringToNumber<uint16_t>(tokens[PC7_PriceListNumber]);
	prices.add(tokens[PC7_PaymentDevice], priceListNum, ConfigPriceIndexType_None);
	LOG_INFO(LOG_EVADTS, "Add price list " << tokens[PC7_PaymentDevice] << priceListNum);
#endif
	return true;
}

bool Config3ConfigIniter::parsePC9(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parsePC9");
	enum {
		PC9_ProductID = 1,
		PC9_CashlessId = 2,
		PC9_TaxRate = 3,
	};
	if(tokenNum < (PC9_CashlessId + 1) || tokens[PC9_CashlessId][0] == '\0') {
		LOG_INFO(LOG_EVADTS, "Ignore PC9");
		return true;
	}

	const char *productId = tokens[PC9_ProductID];
	uint16_t productIndex = products.getIndex(productId);
	if(productIndex == Config3ProductIndexList::UndefinedIndex) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " not exist");
		return false;
	}

	Config3ProductIndex *product = products.get(productIndex);
	uint16_t cashlessId = Sambery::stringToNumber<uint16_t>(tokens[PC9_CashlessId]);
	product->cashlessId = cashlessId;
	LOG_INFO(LOG_EVADTS, "Set product " << productId << " to " << cashlessId);
	return true;
}
