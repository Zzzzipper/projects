#include "Config3AuditParser.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

Config3AuditParser::Config3AuditParser(Config3Modem *config) :
	Evadts::Parser(),
	config(config) {
	product = config->getAutomat()->createIterator();
}

Config3AuditParser::~Config3AuditParser() {
	delete product;
}

void Config3AuditParser::procStart() {
	LOG_DEBUG(LOG_EVADTS, "procStart");
	Config3PriceIndexList *priceLists = config->getAutomat()->getPriceIndexList();
	for(uint16_t i = 0; i < priceLists->getSize(); i++) {
		Config3PriceIndex *index = priceLists->get(i);
		index->type = Config3PriceIndexType_None;
	}
}

void Config3AuditParser::procComplete() {
	LOG_DEBUG(LOG_EVADTS, "complete");
	config->save();
}

bool Config3AuditParser::procLine(char **tokens, uint16_t tokenNum) {
	if(strcmp("AM2", tokens[0]) == 0) {	return parseAM2(tokens, tokenNum); }
	else if(strcmp("AM3", tokens[0]) == 0) { return parseAM3(tokens, tokenNum); }
	else if(strcmp("ID1", tokens[0]) == 0) { return parseID1(tokens, tokenNum); }
	else if(strcmp("ID4", tokens[0]) == 0) { return parseID4(tokens, tokenNum); }
//	else if(strcmp("LA1", tokens[0]) == 0) { return parseLA1(tokens, tokenNum); }
	else if(strcmp("PA1", tokens[0]) == 0) { return parsePA1(tokens, tokenNum); }
	else if(strcmp("PA2", tokens[0]) == 0) { return parsePA2(tokens, tokenNum); }
	else if(strcmp("PA4", tokens[0]) == 0) { return parsePA4(tokens, tokenNum); }
	else if(strcmp("PA7", tokens[0]) == 0) { return parsePA7(tokens, tokenNum); }
	else if(strcmp("PA9", tokens[0]) == 0) { return parsePA9(tokens, tokenNum); }
	else { LOG_DEBUG(LOG_EVADTS, "Ignore line " << tokens[0]); }
	return true;
}

bool Config3AuditParser::parseAM2(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parseAM2");
	enum {
		AM2_IMEI = 1,
		AM2_GprsApn = 2,
		AM2_GprsUsername = 3,
		AM2_GprsPassword = 4,
		AM2_PaymentBus = 5,
		AM2_HardwareVersion = 6,
		AM2_FirmwareVersion = 7,
		AM2_FirmwareAutoUpdate = 8,
		AM2_GsmFirmwareVersion = 9,
	};
	if(tokenNum < (AM2_GsmFirmwareVersion + 1)) {
		LOG_DEBUG(LOG_EVADTS, "Ignore AC2");
		return false;
	}

	config->getBoot()->setImei(tokens[AM2_IMEI]);
	config->getBoot()->setGprsApn(tokens[AM2_GprsApn]);
	config->getBoot()->setGprsUsername(tokens[AM2_GprsUsername]);
	config->getBoot()->setGprsPassword(tokens[AM2_GprsPassword]);
	uint16_t paymentBus = Sambery::stringToNumber<uint16_t>(tokens[AM2_PaymentBus]);
	config->getAutomat()->setPaymentBus(paymentBus);
	uint32_t hardwareVersion = Sambery::stringToNumber<uint32_t>(tokens[AM2_HardwareVersion]);
	config->getBoot()->setHardwareVersion(hardwareVersion);
	uint32_t firmwareVersion = Sambery::stringToNumber<uint32_t>(tokens[AM2_FirmwareVersion]);
	config->getBoot()->setFirmwareVersion(firmwareVersion);
	uint16_t firmwareRelease = Sambery::stringToNumber<uint16_t>(tokens[AM2_FirmwareAutoUpdate]);
#if 0
	config->getBoot()->setFirmwareRelease(firmwareRelease);
#endif
	config->getBoot()->setGsmFirmwareVersion(tokens[AM2_GsmFirmwareVersion]);

	LOG_INFO(LOG_EVADTS, "IMEI to " << tokens[AM2_IMEI]);
	LOG_INFO(LOG_EVADTS, "GprsApn to " << tokens[AM2_GprsApn]);
	LOG_INFO(LOG_EVADTS, "GprsUsername to " << tokens[AM2_GprsUsername]);
	LOG_INFO(LOG_EVADTS, "GprsPassword to " << tokens[AM2_GprsPassword]);
	LOG_INFO(LOG_EVADTS, "PaymentBus to " << paymentBus);
	LOG_INFO(LOG_EVADTS, "HardwareVersion to " << hardwareVersion);
	LOG_INFO(LOG_EVADTS, "FirmwareVersion to " << firmwareVersion);
	LOG_INFO(LOG_EVADTS, "FirmwareRelease to " << firmwareRelease);
	LOG_INFO(LOG_EVADTS, "GsmFirmwareVersion to " << tokens[AM2_GsmFirmwareVersion]);
	return true;
}

bool Config3AuditParser::parseAM3(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parseAM3");
	enum {
		AM3_Kkt = 1,
		AM3_KktInterface = 2,
		AM3_KktIpaddr = 3,
		AM3_KktPort = 4,
		AM3_OfdIpaddr = 5,
		AM3_OfdPort = 6,
	};
	if(tokenNum < (AM3_OfdPort + 1)) {
		LOG_DEBUG(LOG_EVADTS, "Ignore AC3");
		return false;
	}

	uint16_t kkt = Sambery::stringToNumber<uint16_t>(tokens[AM3_Kkt]);
	config->getFiscal()->setKkt(kkt);
	uint16_t kktInterface = Sambery::stringToNumber<uint16_t>(tokens[AM3_KktInterface]);
	config->getFiscal()->setKktInterface(kktInterface);
	config->getFiscal()->setKktAddr(tokens[AM3_KktIpaddr]);
	uint16_t kktPort = Sambery::stringToNumber<uint16_t>(tokens[AM3_KktPort]);
	config->getFiscal()->setKktPort(kktPort);
	config->getFiscal()->setOfdAddr(tokens[AM3_OfdIpaddr]);
	uint16_t ofdPort = Sambery::stringToNumber<uint16_t>(tokens[AM3_OfdPort]);
	config->getFiscal()->setOfdPort(ofdPort);

	LOG_INFO(LOG_EVADTS, "KKT to " << kkt);
	LOG_INFO(LOG_EVADTS, "KktInterface to " << kktInterface);
	LOG_INFO(LOG_EVADTS, "KKT address to " << tokens[AM3_KktIpaddr] << ":" << kktPort);
	LOG_INFO(LOG_EVADTS, "OFD address to " << tokens[AM3_OfdIpaddr] << ":" << ofdPort);
	return true;
}

bool Config3AuditParser::parseID1(char **tokens, uint16_t tokenNum) {
	enum {
		ID1_AutomatId = 6
	};
	if(tokenNum < (ID1_AutomatId + 1)) {
		LOG_WARN(LOG_EVADTS, "ID1 too less params");
		return true;
	}
	config->getAutomat()->setAutomatId(Sambery::stringToNumber<uint32_t>(tokens[ID1_AutomatId]));
	LOG_INFO(LOG_EVADTS, "Set automatId " << tokens[ID1_AutomatId]);
	return true;
}

bool Config3AuditParser::parseID4(char **tokens, uint16_t tokenNum) {
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

#if 0
bool Config3AuditParser::parseLA1(char **tokens, uint16_t tokenNum) {
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
	if(product->find(productId) == false) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " not found");
		return false;
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
#endif

bool Config3AuditParser::parsePA1(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "Found PA1");
	enum {
		PA1_ProductID = 1,
		PA1_ProductPrice = 2,
		PA1_ProductName = 3,
		PA1_ProductDisable = 7
	};
	if(tokenNum < (PA1_ProductID + 1)) {
		LOG_WARN(LOG_EVADTS, "Ignore PA1");
		return false;
	}
	const char *productId = tokens[PA1_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " not found");
		return false;
	}
	if(tokenNum > PA1_ProductPrice) {
		uint32_t price = Sambery::stringToNumber<uint32_t>(tokens[PA1_ProductPrice]);
		product->setPrice("CA", 0, price);
		LOG_INFO(LOG_EVADTS, "Product " << product->getId() << " CA0 price=" << price);
	}
	if(tokenNum > PA1_ProductName) {
		Evadts::latinToWin1251(tokens[PA1_ProductName]);
		product->setName(tokens[PA1_ProductName]);
		LOG_INFO(LOG_EVADTS, "Product " << productId << " name=" << tokens[PA1_ProductName]);
	}
	return true;
}

bool Config3AuditParser::parsePA2(char **tokens, uint16_t tokenNum) {
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

	if(product->isSet() == false) {
		LOG_ERROR(LOG_EVADTS, "Alone tag PA2");
		return false;
	}

	uint32_t saleTotal = Sambery::stringToNumber<uint32_t>(tokens[PA2_SaleTotal]);
	LOG_INFO(LOG_EVADTS, "Product " << product->getId() << " saleTotal=" << saleTotal);
	return true;
}

bool Config3AuditParser::parsePA4(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "Found PA4");
	enum {
		PA4_FreeTotalCount = 1,
		PA4_FreeCount = 3
	};
	uint32_t freeTotalCount = 0;
	if(tokenNum > PA4_FreeTotalCount) {
		freeTotalCount = Sambery::stringToNumber<uint32_t>(tokens[PA4_FreeTotalCount]);
	}
	uint32_t freeCount = 0;
	if(tokenNum > PA4_FreeCount) {
		freeCount = Sambery::stringToNumber<uint32_t>(tokens[PA4_FreeCount]);
	}
	product->setFreeCount(freeTotalCount, freeCount);
	LOG_INFO(LOG_EVADTS, "Product " << product->getId() << " free " << freeTotalCount << ", " << freeCount);
	return true;
}

bool Config3AuditParser::parsePA7(char **tokens, uint16_t tokenNum) {
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
	if(product->findBySelectId(productId) == false) {
		LOG_ERROR(LOG_EVADTS, "Product " << productId << " not found");
		return false;
	}

	const char *paymentDevice = tokens[PA7_PaymentDevice];
	uint16_t priceListNumber = Sambery::stringToNumber<uint32_t>(tokens[PA7_PriceListNumber]);
	uint32_t price = Sambery::stringToNumber<uint32_t>(tokens[PA7_Price]);
	uint32_t saleTotal = Sambery::stringToNumber<uint32_t>(tokens[PA7_SaleTotal]);
	product->setPrice(paymentDevice, priceListNumber, price);

	Config3PriceIndex *priceList = config->getAutomat()->getPriceIndexList()->get(paymentDevice, priceListNumber);
	if(priceList == NULL) {
		LOG_DEBUG(LOG_EVADTS, "Price list " << paymentDevice << priceListNumber << " not found");
		return false;
	}
	if(priceList->type == Config3PriceIndexType_None) {
		priceList->type = Config3PriceIndexType_Base;
	}

	LOG_INFO(LOG_EVADTS, "Product " << productId << " " << paymentDevice << priceListNumber << " price=" << price << ", saleTotal=" << saleTotal);
	return true;
}

bool Config3AuditParser::parsePA9(char **tokens, uint16_t tokenNum) {
	(void)tokenNum;
	LOG_DEBUG(LOG_EVADTS, "parsePA9");
	enum {
		PA9_ProductID = 1,
		PA9_CashlessID = 2,
		PA9_TaxRate = 3,
	};
	if(hasTokenValue(PA9_TaxRate) == false) {
		LOG_DEBUG(LOG_EVADTS, "Ignore PC9");
		return false;
	}

	const char *productId = tokens[PA9_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " not found");
		return false;
	}

	uint32_t taxRate = Sambery::stringToNumber<uint16_t>(tokens[PA9_TaxRate]);
	LOG_INFO(LOG_EVADTS, "Product " << productId << " taxRate=" << tokens[PA9_TaxRate]);
	product->setTaxRate(taxRate);
	return true;
}
