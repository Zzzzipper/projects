#include "Config3AuditFiller.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

FlagContainer::FlagContainer(uint16_t indexMax) : indexMax(indexMax) {
	uint16_t size = indexMax/8 + 1;
	flag = new uint8_t[size];
}

FlagContainer::~FlagContainer() {
	delete flag;
}

void FlagContainer::clear() {
	uint16_t size = indexMax/8 + 1;
	for(uint16_t i = 0; i < size; i++) {
		flag[i] = 0;
	}
}

bool FlagContainer::isExist(uint16_t index) {
	return (index < indexMax);
}

void FlagContainer::setFlag(uint16_t index) {
	uint16_t indexByte = index / 8;
	uint16_t indexBit = index % 8;
	flag[indexByte] = flag[indexByte] | (1 << indexBit);
}

bool FlagContainer::getFlag(uint16_t index) {
	uint16_t indexByte = index / 8;
	uint16_t indexBit = index % 8;
	return (flag[indexByte] & (1 << indexBit));
}

Config3AuditFiller::Config3AuditFiller(Config3Automat *config) :
	Evadts::Parser(),
	config(config),
	flags(256)
{
	product = config->createIterator();
}

Config3AuditFiller::~Config3AuditFiller() {
	delete product;
}

void Config3AuditFiller::procStart() {
	LOG_DEBUG(LOG_EVADTS, "procStart");
	flags.clear();
}

void Config3AuditFiller::procComplete() {
	LOG_DEBUG(LOG_EVADTS, "complete");
	config->save();
}

bool Config3AuditFiller::procLine(char **tokens, uint16_t tokenNum) {
	if(strcmp("ID4", tokens[0]) == 0) { return parseID4(tokens, tokenNum); }
	else if(strcmp("CA15", tokens[0]) == 0) { return parseCA15(tokens, tokenNum); }
	else if(strcmp("CA17", tokens[0]) == 0) { return parseCA17(tokens, tokenNum); }
	else if(strcmp("PA1", tokens[0]) == 0) { return parsePA1(tokens, tokenNum); }
	else if(strcmp("PA2", tokens[0]) == 0) { return parsePA2(tokens, tokenNum); }
	else if(strcmp("PA7", tokens[0]) == 0) { return parsePA7(tokens, tokenNum); }
	else if(strcmp("LA1", tokens[0]) == 0) { return parseLA1(tokens, tokenNum); }
	else { LOG_DEBUG(LOG_EVADTS, "Ignore line " << tokens[0]); }
	return true;
}

bool Config3AuditFiller::parseID4(char **tokens, uint16_t tokenNum) {
	enum {
		ID4_DecimalPoint = 1
	};
	if(tokenNum < (ID4_DecimalPoint + 1)) {
		LOG_WARN(LOG_EVADTS, "ID4 too less params");
		return true;
	}
	config->setDecimalPoint(Sambery::stringToNumber<uint16_t>(tokens[ID4_DecimalPoint]));
	LOG_INFO(LOG_EVADTS, "Set decimalPoint " << tokens[ID4_DecimalPoint]);
	return true;
}

bool Config3AuditFiller::parseCA15(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "Found CA15");
	enum {
		CA15_ValueOfTubeContents = 1,
	};
	if(tokenNum < (CA15_ValueOfTubeContents + 1)) {
		LOG_ERROR(LOG_EVADTS, "Wrong CA15");
		return false;
	}
	uint32_t value = Sambery::stringToNumber<uint32_t>(tokens[CA15_ValueOfTubeContents]);
	config->getCCContext()->setInTubeValue(value);
	return true;
}

bool Config3AuditFiller::parseCA17(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "Found CA17");
	enum {
		CA17_CoinTypeNumber = 1,
		CA17_ValueOfCoin = 2,
		CA17_NumberOfCoinsInTube = 3,
		CA17_ManualFill = 4,
		CA17_ManualInvent = 5,
		CA17_CoinTubeIsFull = 6,
	};
	if(tokenNum < (CA17_NumberOfCoinsInTube + 1)) {
		LOG_ERROR(LOG_EVADTS, "Ignore CA17");
		return true;
	}

	MdbCoinChangerContext *coins = config->getCCContext();
	uint16_t coinIndex = Sambery::stringToNumber<uint16_t>(tokens[CA17_CoinTypeNumber]);
	MdbCoin *coin = coins->get(coinIndex);
	if(coin == NULL) {
		LOG_ERROR(LOG_EVADTS, "Wrong index " << coinIndex);
		return false;
	}

	uint16_t coinNominal = Sambery::stringToNumber<uint16_t>(tokens[CA17_ValueOfCoin]);
	uint16_t coinNumber = Sambery::stringToNumber<uint16_t>(tokens[CA17_NumberOfCoinsInTube]);
	coin->setNominal(coinNominal);
	coin->setNumber(coinNumber);
	coin->setInTube(true);

	uint16_t coinTubeIsFull = false;
	if(tokenNum >= (CA17_CoinTubeIsFull + 1)) {
		coinTubeIsFull = Sambery::stringToNumber<uint16_t>(tokens[CA17_CoinTubeIsFull]);
	}
	coin->setFullTube(coinTubeIsFull > 0);

	LOG_INFO(LOG_EVADTS, "Coin " << coinIndex << " " << coinNominal << "/" << coinNumber << "/" << coinTubeIsFull);
	return true;
}

bool Config3AuditFiller::parsePA1(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "Found PA1");
	enum {
		PA1_ProductID = 1,
		PA1_ProductPrice = 2,
		PA1_ProductName = 3,
		PA1_ProductDisable = 7
	};
	if(tokenNum < (PA1_ProductID + 1)) {
		LOG_WARN(LOG_EVADTS, "Wrong PA1");
		return false;
	}
	const char *productId = tokens[PA1_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " not found");
		return true;
	}
	if(tokenNum > PA1_ProductPrice) {
		const char *last;
		uint32_t price = Sambery::stringToNumber<uint32_t>(tokens[PA1_ProductPrice], &last);
		if(tokens[PA1_ProductPrice] != last) {
			product->setPrice("CA", 0, price);
			LOG_INFO(LOG_EVADTS, "Product " << product->getId() << " CA0 price=" << price);
		}
	}
	return true;
}

bool Config3AuditFiller::parsePA2(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "Found PA2");
	enum {
		PA2_SaleTotal = 1,
		PA2_ValueTotal = 2,
		PA2_SaleSinceReset = 3,
		PA2_ValueSinceReset = 4,
	};
	if(tokenNum < PA2_SaleSinceReset) {
		LOG_ERROR(LOG_EVADTS, "Wrong PA2");
		return false;
	}

	if(product->isSet() == false) {
		LOG_ERROR(LOG_EVADTS, "Alone tag PA2");
		return true;
	}

	uint32_t saleTotal = Sambery::stringToNumber<uint32_t>(tokens[PA2_SaleTotal]);
	uint32_t valueTotal = Sambery::stringToNumber<uint32_t>(tokens[PA2_ValueTotal]);
	uint32_t sale = Sambery::stringToNumber<uint32_t>(tokens[PA2_SaleSinceReset]);
	uint32_t value = Sambery::stringToNumber<uint32_t>(tokens[PA2_ValueSinceReset]);
	product->setCount(saleTotal, valueTotal, sale, value);
	LOG_INFO(LOG_EVADTS, "Product " << product->getId() << " " << saleTotal << "/" << valueTotal << "," << sale << "/" << value);
	return true;
}

bool Config3AuditFiller::parsePA7(char **tokens, uint16_t tokenNum) {
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
		LOG_ERROR(LOG_EVADTS, "Wrong PA7");
		return false;
	}

	const char *productId = tokens[PA7_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_ERROR(LOG_EVADTS, "Product " << productId << " not found");
		return true;
	}

	const char *paymentDevice = tokens[PA7_PaymentDevice];
	uint16_t priceListNumber = Sambery::stringToNumber<uint16_t>(tokens[PA7_PriceListNumber]);
	uint32_t price = Sambery::stringToNumber<uint32_t>(tokens[PA7_Price]);
	uint32_t saleTotal = Sambery::stringToNumber<uint32_t>(tokens[PA7_SaleTotal]);
	uint32_t valueTotal = Sambery::stringToNumber<uint32_t>(tokens[PA7_ValueTotal]);
	uint32_t sale = Sambery::stringToNumber<uint32_t>(tokens[PA7_Sale]);
	uint32_t value = Sambery::stringToNumber<uint32_t>(tokens[PA7_Value]);

	Config3Price *priceList = product->getPrice(paymentDevice, priceListNumber);
	if(priceList == NULL) {
		LOG_ERROR(LOG_EVADTS, "Price list " << paymentDevice << priceListNumber << " of product "  << productId << " not found");
		return true;
	}
	priceList->data.price = price;
	priceList->data.totalCount = saleTotal;
	priceList->data.totalMoney = valueTotal;
	priceList->data.count = sale;
	priceList->data.money = value;
	priceList->save();
	LOG_INFO(LOG_EVADTS, "Product " << productId << " " << paymentDevice << priceListNumber << " price=" << price << ", count=" << saleTotal << "/" << valueTotal << "," << sale << "/" << value);
	return true;
}

bool Config3AuditFiller::parseLA1(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "Found LA1");
	enum {
		LA1_PriceListNumber = 1,
		LA1_ProductID = 2,
		LA1_Price = 3,
		LA1_SaleSinceReset = 4,
		LA1_SaleTotal = 5,
	};
	if(tokenNum < LA1_SaleTotal) {
		LOG_ERROR(LOG_EVADTS, "Wrong LA1");
		return false;
	}

	const char *productId = tokens[LA1_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_ERROR(LOG_EVADTS, "Product " << productId << " not found");
		return true;
	}

	uint16_t priceListNumber = Sambery::stringToNumber<uint16_t>(tokens[LA1_PriceListNumber]);
	uint32_t price = Sambery::stringToNumber<uint32_t>(tokens[LA1_Price]);
	uint32_t saleSinceReset = Sambery::stringToNumber<uint32_t>(tokens[LA1_SaleSinceReset]);
	uint32_t saleTotal = Sambery::stringToNumber<uint32_t>(tokens[LA1_SaleTotal]);

	Config3Price *priceList = NULL;
	if(priceListNumber == 0) {
		priceList = product->getPrice("CA", 0);
	} else if(priceListNumber == 1) {
		priceList = product->getPrice("DA", 1);
	}
	if(priceList == NULL) {
		LOG_ERROR(LOG_EVADTS, "LA1 " << priceListNumber << " of product "  << productId << " not found");
		return true;
	}

	priceList->data.price = price;
	priceList->data.totalCount = saleTotal;
	priceList->data.totalMoney = EvadtsUint32Undefined;
	priceList->data.count = saleSinceReset;
	priceList->data.money = EvadtsUint32Undefined;
	priceList->save();
	LOG_INFO(LOG_EVADTS, "Product " << productId << " LA1=" << priceListNumber << " price=" << price << ", count=" << saleTotal << "/" << saleSinceReset);
	return true;
}
