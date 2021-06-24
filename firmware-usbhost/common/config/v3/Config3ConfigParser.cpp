#include "Config3ConfigParser.h"
#include "dex/include/AuditParser.h"
#include "tcpip/include/TcpIpUtils.h"
#include "utils/include/Number.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

#include <string.h>

Config3ConfigParser::Config3ConfigParser(Config3Modem *config, bool fixedDecimalPoint) :
	Evadts::Parser(),
	config(config),
	fixedDecimalPoint(fixedDecimalPoint),
	devider(1),
	cert(CERT_SIZE, CERT_SIZE)
{
	product = config->getAutomat()->createIterator();
}

Config3ConfigParser::~Config3ConfigParser() {
	delete product;
}

void Config3ConfigParser::setFixedDecimalPoint(bool fixedDecimalPoint) {
	this->fixedDecimalPoint = fixedDecimalPoint;
}

void Config3ConfigParser::procStart() {
	LOG_DEBUG(LOG_EVADTS, "procStart");
	Config3PriceIndexList *priceLists = config->getAutomat()->getPriceIndexList();
	for(uint16_t i = 0; i < priceLists->getSize(); i++) {
		Config3PriceIndex *index = priceLists->get(i);
		index->type = Config3PriceIndexType_None;
	}
	config->getAutomat()->getBarcodeList()->clear();
	config->getAutomat()->getWareGroupList()->clear();
	config->getAutomat()->getWareLinkList()->clear();
}

void Config3ConfigParser::procComplete() {
	LOG_DEBUG(LOG_EVADTS, "complete");
	config->save();
}

bool Config3ConfigParser::procLine(char **tokens, uint16_t tokenNum) {
	if(strcmp("IC1", tokens[0]) == 0) { return parseIC1(tokens); }
	else if(strcmp("IC4", tokens[0]) == 0) { return parseIC4(tokens); }
	else if(strcmp("AC2", tokens[0]) == 0) { return parseAC2(tokens); }
	else if(strcmp("AC3", tokens[0]) == 0) { return parseFC1(tokens); }
	else if(strcmp("FC1", tokens[0]) == 0) { return parseFC1(tokens); }
	else if(strcmp("FC2", tokens[0]) == 0) { return parseFC2(tokens); }
	else if(strcmp("FC3", tokens[0]) == 0) { return parseFC3(tokens); }
	else if(strcmp("FC4", tokens[0]) == 0) { return parseFC4(tokens); }
	else if(strcmp("FC5", tokens[0]) == 0) { return parseFC5(tokens); }
	else if(strcmp("LC2", tokens[0]) == 0) { return parseLC2(tokens); }
	else if(strcmp("PC1", tokens[0]) == 0) { return parsePC1(tokens); }
	else if(strcmp("PC7", tokens[0]) == 0) { return parsePC7(tokens); }
	else if(strcmp("PC9", tokens[0]) == 0) { return parsePC9(tokens, tokenNum); }
	else if(strcmp("MC5", tokens[0]) == 0) { return parseMC5(tokens); }
	else { LOG_DEBUG(LOG_EVADTS, "Ignore line " << tokens[0]); }
	return true;
}

/*
Example of configuration data:
DXS*XYZ1234567*VA*V0/6*1
ST*001*0001
IC1*EPHOR00001*135
// IC4 - описание валюты (обязателен)
// 1 - плавающая точка
// 2 - код валюты ISO 4217
// 3 - описание валюты
// 4 - текстовое обозначение валюты
// 5 - максимальный кредит
IC4*2*0000
// AC2 - параметры телеметрии
// 1 - IMEI (изменять нельзя)
// 2 - GPRS APN
// 3 - GPRS логин
// 4 - GPRS пароль
// 5 - Платежная шина (MDB/EXECUTIVE)
AC2*123456789012345*internet*gdata*gdata*1
// AC3 - параметры фискального регистратора
// 1 - идентификатор ККТ
// 2 - интерфейс ККТ
// 3 - IP-адрес ККТ
// 4 - порт ККТ
// 5 - IP-адрес ОФД
// 6 - порт ОФД
AC3*1*1*192.168.1.201*5555*91.107.67.212*7790
// IC1 - описание автомата
// 1 - серийный номер
// 6 - номер ТА
// PC1 - описание продукта
// 1 - идентификатор продукта (ячейки)
// 2 - цена продукта
// 3 - идентификатор товара
// 4 - максимальная загрузка товара
// 5 - Standard Filling Level
// 6 - Standard Dispensed Quantity
// 7 - Selection status (0 - включена, 1 - отключена)
PC1*10******1
PC1*11**Чипсы****0
// PC7 - цена продукта
// 1 - идентификатор продукта (ячейки)
// 2 - способ оплаты (CA - наличные, DA - картридер 1, DB - картридер 2)
// 3 - номер прайс листа в способе оплаты
// 4 - значение цены
PC7*11*CA*0*3500
PC7*11*DA*1*3500
PC7*11*DA*2*3500
PC7*11*DA*3*3500
G85*1234 (example G85 CRC is 1234)
SE*9*0001
DXE*1*1
*/
bool Config3ConfigParser::parseIC1(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseIC1");
	enum {
		IC1_AutomatId = 6,
	};

	if(hasToken(IC1_AutomatId) == false) {
		LOG_DEBUG(LOG_EVADTS, "Wrong IC1");
		return false;
	}

	uint32_t automatId = Sambery::stringToNumber<uint32_t>(tokens[IC1_AutomatId]);
	config->getAutomat()->setAutomatId(automatId);
	LOG_INFO(LOG_EVADTS, "AutomatId to " << automatId);
	return true;
}

bool Config3ConfigParser::parseIC4(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseIC4");
	enum {
		IC4_DecimalPlaces = 1,
		IC4_Currency = 2,
		IC4_TaxSystem = 4,
		IC4_MaxCredit = 5,
		IC4_Ext1Device = 6,
		IC4_Evadts = 7,
		IC4_CashlessNum = 8,
		IC4_ScaleFactor = 9,
		IC4_CategoryMoney = 10,
		IC4_ShowChange = 11,
		IC4_PriceHolding = 12,
		IC4_MultiVend = 13,
		IC4_CreditHolding = 14,
		IC4_Cashless2Click = 15,
		IC4_FiscalPrinter = 16,
		IC4_CashlessMaxLevel = 17,
	};

	if(hasTokenValue(IC4_DecimalPlaces) == true) {
		uint32_t decimalPoint = Sambery::stringToNumber<uint32_t>(tokens[IC4_DecimalPlaces]);
		if(fixedDecimalPoint == false) {
			config->getAutomat()->setDecimalPoint(decimalPoint);
			LOG_INFO(LOG_EVADTS, "DecimalPoint to " << decimalPoint);
		} else {
			if(setDevider(decimalPoint) == false) { return false; }
		}
	}
	if(hasToken(IC4_Currency) == true) {
		if(fixedDecimalPoint == false) {
			uint16_t currency = Sambery::stringToNumber<uint16_t>(tokens[IC4_Currency]);
			config->getAutomat()->setCurrency(currency);
			LOG_INFO(LOG_EVADTS, "Currency to " << config->getAutomat()->getCurrency());
		}
	}
	if(hasTokenValue(IC4_TaxSystem) == true) {
		uint32_t taxSystem = Sambery::stringToNumber<uint32_t>(tokens[IC4_TaxSystem]);
		config->getAutomat()->setTaxSystem(taxSystem);
		LOG_INFO(LOG_EVADTS, "TaxSystem to " << config->getAutomat()->getTaxSystem());
	}
	if(hasTokenValue(IC4_MaxCredit) == true) {
		uint32_t maxCredit = Sambery::stringToNumber<uint32_t>(tokens[IC4_MaxCredit]);
		config->getAutomat()->setMaxCredit(convertValue(maxCredit));
		LOG_INFO(LOG_EVADTS, "MaxCredit to " << config->getAutomat()->getMaxCredit());
	}
	if(hasTokenValue(IC4_Ext1Device) == true) {
		uint32_t ext1Device = Sambery::stringToNumber<uint32_t>(tokens[IC4_Ext1Device]);
		config->getAutomat()->setExt1Device(ext1Device);
		LOG_INFO(LOG_EVADTS, "Ext1Device to " << config->getAutomat()->getExt1Device());
	}
	if(hasTokenValue(IC4_Evadts) == true) {
		uint32_t evadts = Sambery::stringToNumber<uint32_t>(tokens[IC4_Evadts]);
		config->getAutomat()->setEvadts(evadts);
		LOG_INFO(LOG_EVADTS, "Evadts to " << config->getAutomat()->getEvadts());
	}
	if(hasTokenValue(IC4_CashlessNum) == true) {
		uint32_t cashlessNum = Sambery::stringToNumber<uint32_t>(tokens[IC4_CashlessNum]);
		config->getAutomat()->setCashlessNumber(cashlessNum);
		LOG_INFO(LOG_EVADTS, "CashlessNum to " << config->getAutomat()->getCashlessNumber());
	}
	if(hasTokenValue(IC4_ScaleFactor) == true) {
		uint32_t scaleFactor = Sambery::stringToNumber<uint32_t>(tokens[IC4_ScaleFactor]);
		config->getAutomat()->setScaleFactor(scaleFactor);
		LOG_INFO(LOG_EVADTS, "ScaleFactor to " << config->getAutomat()->getScaleFactor());
	}
	if(hasTokenValue(IC4_CategoryMoney) == true) {
		uint32_t categoryMoney = Sambery::stringToNumber<uint32_t>(tokens[IC4_CategoryMoney]);
		config->getAutomat()->setCategoryMoney(categoryMoney);
		LOG_INFO(LOG_EVADTS, "CategoryMoney to " << config->getAutomat()->getCategoryMoney());
	}
	if(hasTokenValue(IC4_ShowChange) == true) {
		uint32_t showChange = Sambery::stringToNumber<uint32_t>(tokens[IC4_ShowChange]);
		config->getAutomat()->setShowChange(showChange);
		LOG_INFO(LOG_EVADTS, "ShowChange to " << config->getAutomat()->getShowChange());
	}
	if(hasTokenValue(IC4_PriceHolding) == true) {
		uint32_t priceHolding = Sambery::stringToNumber<uint32_t>(tokens[IC4_PriceHolding]);
		config->getAutomat()->setPriceHolding(priceHolding);
		LOG_INFO(LOG_EVADTS, "PriceHodling to " << config->getAutomat()->getPriceHolding());
	}
	if(hasTokenValue(IC4_MultiVend) == true) {
		uint32_t multiVend = Sambery::stringToNumber<uint32_t>(tokens[IC4_MultiVend]);
		config->getAutomat()->setMultiVend(multiVend);
		LOG_INFO(LOG_EVADTS, "MultiVend to " << config->getAutomat()->getMultiVend());
	}
	if(hasTokenValue(IC4_CreditHolding) == true) {
		uint32_t creditHolding = Sambery::stringToNumber<uint32_t>(tokens[IC4_CreditHolding]);
		config->getAutomat()->setCreditHolding(creditHolding);
		LOG_INFO(LOG_EVADTS, "CreditHolding to " << config->getAutomat()->getCreditHolding());
	}
	if(hasTokenValue(IC4_Cashless2Click) == true) {
		uint32_t cashless2Click = Sambery::stringToNumber<uint32_t>(tokens[IC4_Cashless2Click]);
		config->getAutomat()->setCashless2Click(cashless2Click);
		LOG_INFO(LOG_EVADTS, "Cashless2Click to " << config->getAutomat()->getCashless2Click());
	}
	if(hasTokenValue(IC4_FiscalPrinter) == true) {
		uint32_t qrCode = Sambery::stringToNumber<uint32_t>(tokens[IC4_FiscalPrinter]);
		config->getAutomat()->setFiscalPrinter(qrCode);
		LOG_INFO(LOG_EVADTS, "FiscalPrinter to " << config->getAutomat()->getFiscalPrinter());
	}
	if(hasTokenValue(IC4_CashlessMaxLevel) == true) {
		uint32_t cashlessMaxLevel = Sambery::stringToNumber<uint32_t>(tokens[IC4_CashlessMaxLevel]);
		config->getAutomat()->setCashlessMaxLevel(cashlessMaxLevel);
		LOG_INFO(LOG_EVADTS, "CashlessMaxLevel to " << config->getAutomat()->getCashlessMaxLevel());
	}
	return true;
}

bool Config3ConfigParser::parseAC2(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseAC2");
	enum {
		AC2_IMEI = 1,
		AC2_GprsApn = 2,
		AC2_GprsUsername = 3,
		AC2_GprsPassword = 4,
		AC2_PaymentBus = 5,
		AC2_HardwareVersion = 6,
		AC2_FirmwareVersion = 7,
		AC2_FirmwareRelease = 8,
		AC2_GsmFirmwareVersion = 9,
	};
	if(hasTokenValue(AC2_IMEI) == true) {
		config->getBoot()->setImei(tokens[AC2_IMEI]);
		LOG_INFO(LOG_EVADTS, "IMEI to " << tokens[AC2_IMEI]);
	}
	if(hasTokenValue(AC2_GprsApn) == true) {
		config->getBoot()->setGprsApn(tokens[AC2_GprsApn]);
		LOG_INFO(LOG_EVADTS, "GprsApn to " << tokens[AC2_GprsApn]);
	}
	if(hasTokenValue(AC2_GprsUsername) == true) {
		config->getBoot()->setGprsUsername(tokens[AC2_GprsUsername]);
		LOG_INFO(LOG_EVADTS, "GprsUsername to " << tokens[AC2_GprsUsername]);
	}
	if(hasTokenValue(AC2_GprsPassword) == true) {
		config->getBoot()->setGprsPassword(tokens[AC2_GprsPassword]);
		LOG_INFO(LOG_EVADTS, "GprsPassword to " << tokens[AC2_GprsPassword]);
	}
	if(hasTokenValue(AC2_PaymentBus) == true) {
		uint16_t paymentBus = Sambery::stringToNumber<uint16_t>(tokens[AC2_PaymentBus]);
		config->getAutomat()->setPaymentBus(paymentBus);
		LOG_INFO(LOG_EVADTS, "PaymentBus to " << paymentBus);
	}
	if(hasTokenValue(AC2_HardwareVersion) == true) {
		uint32_t hardwareVersion = Sambery::stringToNumber<uint32_t>(tokens[AC2_HardwareVersion]);
		config->getBoot()->setHardwareVersion(hardwareVersion);
		LOG_INFO(LOG_EVADTS, "HardwareVersion to " << hardwareVersion);
	}
	if(hasTokenValue(AC2_FirmwareVersion) == true) {
		uint32_t firmwareVersion = Sambery::stringToNumber<uint32_t>(tokens[AC2_FirmwareVersion]);
		config->getBoot()->setFirmwareVersion(firmwareVersion);
		LOG_INFO(LOG_EVADTS, "FirmwareVersion to " << firmwareVersion);
	}
#if 0
	if(hasTokenValue(AC2_FirmwareRelease) == true) {
		uint16_t firmwareRelease = Sambery::stringToNumber<uint16_t>(tokens[AC2_FirmwareRelease]);
		config->getBoot()->setFirmwareRelease(firmwareRelease);
		LOG_INFO(LOG_EVADTS, "FirmwareRelease to " << firmwareRelease);
	}
#endif
	if(hasTokenValue(AC2_GsmFirmwareVersion) == true) {
		config->getBoot()->setGsmFirmwareVersion(tokens[AC2_GsmFirmwareVersion]);
		LOG_INFO(LOG_EVADTS, "GsmFirmwareVersion to " << tokens[AC2_GsmFirmwareVersion]);
	}
	return true;
}

bool Config3ConfigParser::parseFC1(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseFC1");
	enum {
		FC1_Kkt = 1,
		FC1_KktInterface = 2,
		FC1_KktIpaddr = 3,
		FC1_KktPort = 4,
		FC1_OfdIpaddr = 5,
		FC1_OfdPort = 6,
	};
	if(hasToken(FC1_Kkt) == true) {
		uint16_t kkt = Sambery::stringToNumber<uint16_t>(tokens[FC1_Kkt]);
		config->getFiscal()->setKkt(kkt);
		LOG_INFO(LOG_EVADTS, "KKT to " << kkt);
	}
	if(hasToken(FC1_KktInterface) == true) {
		uint16_t kktInterface = Sambery::stringToNumber<uint16_t>(tokens[FC1_KktInterface]);
		config->getFiscal()->setKktInterface(kktInterface);
		LOG_INFO(LOG_EVADTS, "KktInterface to " << kktInterface);
	}
	if(hasToken(FC1_KktPort) == true) {
		config->getFiscal()->setKktAddr(tokens[FC1_KktIpaddr]);
		uint16_t kktPort = Sambery::stringToNumber<uint16_t>(tokens[FC1_KktPort]);
		config->getFiscal()->setKktPort(kktPort);
		LOG_INFO(LOG_EVADTS, "KKT address to " << tokens[FC1_KktIpaddr] << ":" << kktPort);
	}
	if(hasToken(FC1_OfdPort) == true) {
		config->getFiscal()->setOfdAddr(tokens[FC1_OfdIpaddr]);
		uint16_t ofdPort = Sambery::stringToNumber<uint16_t>(tokens[FC1_OfdPort]);
		config->getFiscal()->setOfdPort(ofdPort);
		LOG_INFO(LOG_EVADTS, "OFD address to " << tokens[FC1_OfdIpaddr] << ":" << ofdPort);
	}
	return true;
}

bool Config3ConfigParser::parseFC2(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseFC2");
	enum {
		FC2_INN = 1,
		FC2_AutomatNumber = 2,
		FC2_PointName = 3,
		FC2_PointAddr = 4,
		FC2_Group = 5,
	};
	if(hasToken(FC2_PointAddr) == false) {
		LOG_DEBUG(LOG_EVADTS, "Wrong FC2");
		return false;
	}

	config->getFiscal()->setINN(tokens[FC2_INN]);
	config->getFiscal()->setAutomatNumber(tokens[FC2_AutomatNumber]);
	config->getFiscal()->setPointName(tokens[FC2_PointName]);
	config->getFiscal()->setPointAddr(tokens[FC2_PointAddr]);
	LOG_INFO(LOG_EVADTS, "INN to " << tokens[FC2_INN]);
	LOG_INFO(LOG_EVADTS, "AutomatNumber to " << tokens[FC2_AutomatNumber]);
	LOG_INFO(LOG_EVADTS, "PointName to " << tokens[FC2_PointName]);
	LOG_INFO(LOG_EVADTS, "PointAddr to " << tokens[FC2_PointAddr]);

	if(hasToken(FC2_Group) == true) {
		config->getFiscal()->setGroup(tokens[FC2_Group]);
		LOG_INFO(LOG_EVADTS, "Group to " << tokens[FC2_Group]);
	}
	return true;
}

bool Config3ConfigParser::parseFC3(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseFC3");
	enum {
		FC3_CertData = 1,
	};
	if(hasToken(FC3_CertData) == false) {
		LOG_DEBUG(LOG_EVADTS, "Wrong FC3");
		return false;
	}
	const char prefixBegin[] = "-----BEGIN";
	uint16_t prefixBeginLen = sizeof(prefixBegin) - 1;
	const char prefixEnd[] = "-----END";
	uint16_t prefixEndLen = sizeof(prefixEnd) - 1;
	const char *fragment = tokens[FC3_CertData];
	if(fragment[0] == '\0') {
		cert.clear();
		config->getFiscal()->setAuthPublicKey(&cert);
	} else if(strncmp(fragment, prefixBegin, prefixBeginLen) == 0) {
		cert.clear();
		cert.addStr(fragment);
		cert.add('\n');
	} else if(strncmp(fragment, prefixEnd, prefixEndLen) == 0) {
		cert.addStr(fragment);
		cert.add('\n');
		config->getFiscal()->setAuthPublicKey(&cert);
	} else {
		cert.addStr(fragment);
		cert.add('\n');
	}
	return true;
}

bool Config3ConfigParser::parseFC4(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseFC4");
	enum {
		FC4_CertData = 1,
	};
	if(hasToken(FC4_CertData) == false) {
		LOG_DEBUG(LOG_EVADTS, "Wrong FC4");
		return false;
	}
	const char prefixBegin[] = "-----BEGIN";
	uint16_t prefixBeginLen = sizeof(prefixBegin) - 1;
	const char prefixEnd[] = "-----END";
	uint16_t prefixEndLen = sizeof(prefixEnd) - 1;
	const char *fragment = tokens[FC4_CertData];
	if(fragment[0] == '\0') {
		cert.clear();
		config->getFiscal()->setAuthPrivateKey(&cert);
	} else if(strncmp(fragment, prefixBegin, prefixBeginLen) == 0) {
		cert.clear();
		cert.addStr(fragment);
		cert.add('\n');
	} else if(strncmp(fragment, prefixEnd, prefixEndLen) == 0) {
		cert.addStr(fragment);
		cert.add('\n');
		config->getFiscal()->setAuthPrivateKey(&cert);
	} else {
		cert.addStr(fragment);
		cert.add('\n');
	}
	return true;
}

bool Config3ConfigParser::parseFC5(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseFC5");
	enum {
		FC5_CertData = 1,
	};
	if(hasToken(FC5_CertData) == false) {
		LOG_DEBUG(LOG_EVADTS, "Wrong FC5");
		return false;
	}
	if(config->getFiscal()->getKkt() == Config2Fiscal::Kkt_Nanokassa) {
		cert.clear();
		cert.addStr(tokens[FC5_CertData]);
		config->getFiscal()->setSignPrivateKey(&cert);
		return true;
	}
	const char prefixBegin[] = "-----BEGIN";
	uint16_t prefixBeginLen = sizeof(prefixBegin) - 1;
	const char prefixEnd[] = "-----END";
	uint16_t prefixEndLen = sizeof(prefixEnd) - 1;
	const char *fragment = tokens[FC5_CertData];
	if(fragment[0] == '\0') {
		cert.clear();
		config->getFiscal()->setSignPrivateKey(&cert);
	} else if(strncmp(fragment, prefixBegin, prefixBeginLen) == 0) {
		cert.clear();
		cert.addStr(fragment);
		cert.add('\n');
	} else if(strncmp(fragment, prefixEnd, prefixEndLen) == 0) {
		cert.addStr(fragment);
		cert.add('\n');
		config->getFiscal()->setSignPrivateKey(&cert);
	} else {
		cert.addStr(fragment);
		cert.add('\n');
	}
	return true;
}

bool Config3ConfigParser::parseLC2(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseLC2");
	enum {
		LC2_PaymentDevice = 1,
		LC2_PriceListNumber = 2,
		LC2_WorkWeek = 3,
		LC2_WorkTime = 4,
		LC2_WorkInterval = 5,
	};
	if(hasToken(LC2_WorkInterval) == false) {
		LOG_DEBUG(LOG_EVADTS, "Wrong LC2");
		return false;
	}

	const char *paymentDevice = tokens[LC2_PaymentDevice];
	uint16_t priceListNumber = Sambery::stringToNumber<uint32_t>(tokens[LC2_PriceListNumber]);
	Config3PriceIndex *priceList = config->getAutomat()->getPriceIndexList()->get(paymentDevice, priceListNumber);
	if(priceList == NULL) {
		LOG_DEBUG(LOG_EVADTS, "Price list " << paymentDevice << priceListNumber << " not found");
		return false;
	}

	uint8_t workWeek = Sambery::stringToNumber<uint8_t>(tokens[LC2_WorkWeek]);
	priceList->timeTable.setWeekTable(workWeek);
	Time workTime;
	if(stringToTime(tokens[LC2_WorkTime], &workTime) == false) {
		LOG_DEBUG(LOG_EVADTS, "Wrong time format");
		return false;
	}

	uint32_t workInterval = Sambery::stringToNumber<uint32_t>(tokens[LC2_WorkInterval]);
	priceList->timeTable.setInterval(&workTime, workInterval);
	priceList->type = Config3PriceIndexType_Time;
	return true;
}

bool Config3ConfigParser::parsePC1(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parsePC1");
	enum {
		PC1_ProductID = 1,
		PC1_ProductPrice = 2,
		PC1_ProductName = 3,
	};
	if(hasToken(PC1_ProductName) == false) {
		LOG_ERROR(LOG_EVADTS, "Wrong PC1");
		return false;
	}

	const char *productId = tokens[PC1_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_ERROR(LOG_EVADTS, "Product " << productId << " not found");
		return false;
	}

	LOG_INFO(LOG_EVADTS, "Product " << productId << " to " << tokens[PC1_ProductName]);
	Evadts::latinToWin1251(tokens[PC1_ProductName]);
	product->setName(tokens[PC1_ProductName]);
	return true;
}

bool Config3ConfigParser::parsePC7(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parsePC7");
	enum {
		PC7_ProductID = 1,
		PC7_PaymentDevice = 2,
		PC7_PriceListNumber = 3,
		PC7_Price = 4,
	};
	if(hasToken(PC7_Price) == false) {
		LOG_DEBUG(LOG_EVADTS, "Ignore PC7");
		return false;
	}

	const char *productId = tokens[PC7_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " not found");
		return false;
	}

	const char *paymentDevice = tokens[PC7_PaymentDevice];
	uint16_t priceListNumber = Sambery::stringToNumber<uint32_t>(tokens[PC7_PriceListNumber]);
	uint32_t price = Sambery::stringToNumber<uint32_t>(tokens[PC7_Price]);
	product->setPrice(paymentDevice, priceListNumber, convertValue(price));

	Config3PriceIndex *priceList = config->getAutomat()->getPriceIndexList()->get(paymentDevice, priceListNumber);
	if(priceList == NULL) {
		LOG_DEBUG(LOG_EVADTS, "Price list " << paymentDevice << priceListNumber << " not found");
		return false;
	}
	if(priceList->type == Config3PriceIndexType_None) {
		priceList->type = Config3PriceIndexType_Base;
	}

	LOG_INFO(LOG_EVADTS, "Product " << productId << " " << paymentDevice << priceListNumber << " price=" << price);
	return true;
}

bool Config3ConfigParser::parsePC9(char **tokens, uint16_t tokenNum) {
	LOG_DEBUG(LOG_EVADTS, "parsePC9");
	enum {
		PC9_ProductID = 1,
		PC9_CashlessID = 2,
		PC9_TaxRate = 3,
		PC9_WareId = 4,
	};

	if(hasTokenValue(PC9_TaxRate) == false) {
		LOG_DEBUG(LOG_EVADTS, "Ignore PC9");
		return false;
	}

	const char *productId = tokens[PC9_ProductID];
	if(product->findBySelectId(productId) == false) {
		LOG_WARN(LOG_EVADTS, "Product " << productId << " not found");
		return false;
	}

	if(tokenNum > PC9_TaxRate) {
		uint32_t taxRate = Sambery::stringToNumber<uint16_t>(tokens[PC9_TaxRate]);
		product->setTaxRate(taxRate);
		LOG_INFO(LOG_EVADTS, "Product " << productId << " taxRate=" << tokens[PC9_TaxRate]);
	}

	if(tokenNum > PC9_WareId) {
		uint32_t wareId = Sambery::stringToNumber<uint16_t>(tokens[PC9_WareId]);
		product->setWareId(wareId);
		LOG_INFO(LOG_EVADTS, "Product " << productId << " wareId=" << tokens[PC9_WareId]);
	}

	return true;
}

bool Config3ConfigParser::parseMC5(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5");
	enum {
		MC5_Order = 1,
		MC5_Id = 2,
	};

	if(hasTokenValue(MC5_Id) == false) {
		LOG_DEBUG(LOG_EVADTS, "Ignore MC5");
		return false;
	}

	if(strcmp("INTERNET", tokens[MC5_Id]) == 0) { return parseMC5_INTERNET(tokens); }
	else if(strcmp("EXT1", tokens[MC5_Id]) == 0) { return parseMC5_EXT1(tokens); }
	else if(strcmp("EXT2", tokens[MC5_Id]) == 0) { return parseMC5_EXT2(tokens); }
	else if(strcmp("USB1", tokens[MC5_Id]) == 0) { return parseMC5_USB1(tokens); }
	else if(strcmp("QRTYPE", tokens[MC5_Id]) == 0) { return parseMC5_QRTYPE(tokens); }
	else if(strcmp("ETH1MAC", tokens[MC5_Id]) == 0) { return parseMC5_ETH1MAC(tokens); }
	else if(strcmp("ETH1ADDR", tokens[MC5_Id]) == 0) { return parseMC5_ETH1ADDR(tokens); }
	else if(strcmp("ETH1MASK", tokens[MC5_Id]) == 0) { return parseMC5_ETH1MASK(tokens); }
	else if(strcmp("ETH1GW", tokens[MC5_Id]) == 0) { return parseMC5_ETH1GW(tokens); }
	else if(strcmp("FIDTYPE", tokens[MC5_Id]) == 0) { return parseMC5_FIDTYPE(tokens); }
	else if(strcmp("FIDADDR", tokens[MC5_Id]) == 0) { return parseMC5_FIDADDR(tokens); }
	else if(strcmp("FIDDEVICE", tokens[MC5_Id]) == 0) { return parseMC5_FIDDEVICE(tokens); }
	else if(strcmp("FIDAUTH", tokens[MC5_Id]) == 0) { return parseMC5_FIDAUTH(tokens); }
	else if(strcmp("BC", tokens[MC5_Id]) == 0) { return parseMC5_BC(tokens); }
	else if(strcmp("BV", tokens[MC5_Id]) == 0) { return parseMC5_BV(tokens); }
	else if(strcmp("WG", tokens[MC5_Id]) == 0) { return parseMC5_WG(tokens); }
	else if(strcmp("WL", tokens[MC5_Id]) == 0) { return parseMC5_WL(tokens); }
	else { LOG_ERROR(LOG_EVADTS, "Ignore line " << tokens[MC5_Id]); }
	return true;
}

bool Config3ConfigParser::parseMC5_INTERNET(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_INTERNET");
	enum {
		MC5_InternetDvice = 3,
	};
	if(hasTokenValue(MC5_InternetDvice) == true) {
		uint16_t internetDevice = Sambery::stringToNumber<uint16_t>(tokens[MC5_InternetDvice]);
		config->getAutomat()->setInternetDevice(internetDevice);
		LOG_INFO(LOG_EVADTS, "InternetDevice to " << internetDevice);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_EXT1(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_EXT1");
	enum {
		MC5_Ext1 = 3,
	};
	if(hasTokenValue(MC5_Ext1) == true) {
		uint16_t ext1Device = Sambery::stringToNumber<uint16_t>(tokens[MC5_Ext1]);
		config->getAutomat()->setExt1Device(ext1Device);
		LOG_INFO(LOG_EVADTS, "Ext1Device to " << ext1Device);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_EXT2(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_EXT2");
	enum {
		MC5_Ext2 = 3,
	};
	if(hasTokenValue(MC5_Ext2) == true) {
		uint16_t ext2Device = Sambery::stringToNumber<uint16_t>(tokens[MC5_Ext2]);
		config->getAutomat()->setExt2Device(ext2Device);
		LOG_INFO(LOG_EVADTS, "Ext2Device to " << ext2Device);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_USB1(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_USB1");
	enum {
		MC5_Usb1 = 3,
	};
	if(hasTokenValue(MC5_Usb1) == true) {
		uint16_t usb1Device = Sambery::stringToNumber<uint16_t>(tokens[MC5_Usb1]);
		config->getAutomat()->setUsb1Device(usb1Device);
		LOG_INFO(LOG_EVADTS, "Usb1Device to " << usb1Device);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_QRTYPE(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_QRTYPE");
	enum {
		MC5_QrType = 3,
	};
	if(hasTokenValue(MC5_QrType) == true) {
		uint16_t qrType = Sambery::stringToNumber<uint16_t>(tokens[MC5_QrType]);
		config->getAutomat()->setQrType(qrType);
		LOG_INFO(LOG_EVADTS, "QrType to " << qrType);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_ETH1MAC(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_ETH1MAC");
	enum {
		MC5_ETHMAC = 3,
	};
	if(hasTokenValue(MC5_ETHMAC) == true) {
		uint8_t mac[6];
		uint16_t macLen = hexToData(tokens[MC5_ETHMAC], strlen(tokens[MC5_ETHMAC]), mac, sizeof(mac));
		if(macLen != config->getAutomat()->getEthMacLen()) { return false; }
		config->getAutomat()->setEthMac(mac);
		LOG_INFO(LOG_EVADTS, "Eth1Mac to " << tokens[MC5_ETHMAC]);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_ETH1ADDR(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_ETH1ADDR");
	enum {
		MC5_ETHMAC = 3,
	};
	if(hasTokenValue(MC5_ETHMAC) == true) {
		uint32_t value;
		if(stringToIpAddr(tokens[MC5_ETHMAC], &value) == false) { return false; }
		config->getAutomat()->setEthAddr(value);
		LOG_INFO(LOG_EVADTS, "Eth1Addr to " << tokens[MC5_ETHMAC]);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_ETH1MASK(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_ETH1MASK");
	enum {
		MC5_ETHMAC = 3,
	};
	if(hasTokenValue(MC5_ETHMAC) == true) {
		uint32_t value;
		if(stringToIpAddr(tokens[MC5_ETHMAC], &value) == false) { return false; }
		config->getAutomat()->setEthMask(value);
		LOG_INFO(LOG_EVADTS, "Eth1Mask to " << tokens[MC5_ETHMAC]);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_ETH1GW(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_ETH1GW");
	enum {
		MC5_ETHMAC = 3,
	};
	if(hasTokenValue(MC5_ETHMAC) == true) {
		uint32_t value;
		if(stringToIpAddr(tokens[MC5_ETHMAC], &value) == false) { return false; }
		config->getAutomat()->setEthGateway(value);
		LOG_INFO(LOG_EVADTS, "Eth1Gateway to " << tokens[MC5_ETHMAC]);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_FIDTYPE(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_FIDTYPE");
	enum {
		MC5_FIDTYPE = 3,
	};
	if(hasTokenValue(MC5_FIDTYPE) == true) {
		uint16_t type = Sambery::stringToNumber<uint16_t>(tokens[MC5_FIDTYPE]);
		config->getAutomat()->setFidType(type);
		LOG_INFO(LOG_EVADTS, "FidType to " << type);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_FIDADDR(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_FIDADDR");
	enum {
		MC5_FIDADDR = 3,
		MC5_FIDPORT = 4,
	};
	if(hasTokenValue(MC5_FIDADDR) == true) {
		config->getAutomat()->setFidAddr(tokens[MC5_FIDADDR]);
		LOG_INFO(LOG_EVADTS, "FidAddr to " << tokens[MC5_FIDADDR]);
	}
	if(hasTokenValue(MC5_FIDPORT) == true) {
		uint16_t port = Sambery::stringToNumber<uint16_t>(tokens[MC5_FIDPORT]);
		config->getAutomat()->setFidPort(port);
		LOG_INFO(LOG_EVADTS, "FidPort to " << port);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_FIDDEVICE(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_FIDDEVICE");
	enum {
		MC5_FIDDEVICE = 3,
	};
	if(hasTokenValue(MC5_FIDDEVICE) == true) {
		config->getAutomat()->setFidDevice(tokens[MC5_FIDDEVICE]);
		LOG_INFO(LOG_EVADTS, "FidDevice to " << tokens[MC5_FIDDEVICE]);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_FIDAUTH(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_FIDAUTH");
	enum {
		MC5_FIDUSERNAME = 3,
		MC5_FIDPASSWORD = 4,
	};
	if(hasTokenValue(MC5_FIDUSERNAME) == true) {
		config->getAutomat()->setFidUsername(tokens[MC5_FIDUSERNAME]);
		LOG_INFO(LOG_EVADTS, "FidUsername to " << tokens[MC5_FIDUSERNAME]);
	}
	if(hasTokenValue(MC5_FIDPASSWORD) == true) {
		config->getAutomat()->setFidPassword(tokens[MC5_FIDPASSWORD]);
		LOG_INFO(LOG_EVADTS, "FidPassword to " << tokens[MC5_FIDPASSWORD]);
	}
	return true;
}

bool Config3ConfigParser::parseMC5_BC(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_BC");
	enum {
		MC5_BC_WAREID = 3,
		MC5_BC_BARCODE = 4,
	};
	if(hasTokenValue(MC5_BC_WAREID) == false || hasTokenValue(MC5_BC_BARCODE) == false) {
		return true;
	}
	uint32_t wareId = Sambery::stringToNumber<uint32_t>(tokens[MC5_BC_WAREID]);
	if(wareId == 0) {
		return true;
	}
	config->getAutomat()->getBarcodeList()->add(wareId, tokens[MC5_BC_BARCODE]);
	return true;
}

bool Config3ConfigParser::parseMC5_BV(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_BV");
	enum {
		MC5_BV_ON = 3,
	};
	if(hasTokenValue(MC5_BV_ON) == true) {
		uint32_t on = Sambery::stringToNumber<uint32_t>(tokens[MC5_BV_ON]);
		config->getAutomat()->getBVContext()->setUseChange(on);
		LOG_INFO(LOG_EVADTS, "BV to " << tokens[MC5_BV_ON]);
		return true;
	}
	return true;
}

bool Config3ConfigParser::parseMC5_WG(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_WG");
	enum {
		MC5_BC_GROUPID = 3,
		MC5_BC_GROUPPARENTID = 4,
		MC5_BC_GROUPNAME = 5,
		MC5_BC_GROUPCOLOR = 6,
	};
	if(hasTokenValue(MC5_BC_GROUPID) == false || hasTokenValue(MC5_BC_GROUPNAME) == false) {
		return true;
	}
	uint32_t groupId = Sambery::stringToNumber<uint32_t>(tokens[MC5_BC_GROUPID]);
	if(groupId == 0) {
		return true;
	}
	uint32_t parentId = Sambery::stringToNumber<uint32_t>(tokens[MC5_BC_GROUPPARENTID]);
	Evadts::latinToWin1251(tokens[MC5_BC_GROUPNAME]);
	config->getAutomat()->getWareGroupList()->add(groupId, parentId, tokens[MC5_BC_GROUPNAME], tokens[MC5_BC_GROUPCOLOR]);
	LOG_INFO(LOG_EVADTS, "Add WareGroup " << groupId << "/" << parentId << "/" << tokens[MC5_BC_GROUPNAME]);
	return true;
}

bool Config3ConfigParser::parseMC5_WL(char **tokens) {
	LOG_DEBUG(LOG_EVADTS, "parseMC5_WL");
	enum {
		MC5_BC_GROUPID = 3,
		MC5_BC_WAREID = 4,
	};
	if(hasTokenValue(MC5_BC_GROUPID) == false || hasTokenValue(MC5_BC_WAREID) == false) {
		return true;
	}
	uint32_t groupId = Sambery::stringToNumber<uint32_t>(tokens[MC5_BC_GROUPID]);
	if(groupId == 0) {
		return true;
	}
	uint32_t wareId = Sambery::stringToNumber<uint32_t>(tokens[MC5_BC_WAREID]);
	if(wareId == 0) {
		return true;
	}
	config->getAutomat()->getWareLinkList()->add(groupId, wareId);
	LOG_INFO(LOG_EVADTS, "Add WareGroup " << groupId << "/" << wareId);
	return true;
}

bool Config3ConfigParser::setDevider(uint32_t decimalPoint) {
	uint32_t configDecimalPoint = config->getAutomat()->getDecimalPoint();
	LOG_DEBUG(LOG_EVADTS, "setDevider " << configDecimalPoint << ":" << decimalPoint);
	if(decimalPoint != 2) {
		LOG_ERROR(LOG_EVADTS, "Decimal point must be equal 2 (" << decimalPoint << ")");
		return false;
	}
	uint32_t diffDecimalPoint = decimalPoint - configDecimalPoint;
	devider = 1;
	for(uint32_t i = 0; i < diffDecimalPoint; i++) {
		devider = devider * 10;
	}
	return true;
}

uint32_t Config3ConfigParser::convertValue(uint32_t value) {
	return (value / devider);
}
