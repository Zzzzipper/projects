#include "Config4ConfigGenerator.h"
#include "dex/DexProtocol.h"
#include "logger/include/Logger.h"

Config4ConfigGenerator::Config4ConfigGenerator(Config4Modem *config) :
	EvadtsGenerator(config->getAutomat()->getCommunictionId()),
	config(config),
	cert(CERT_SIZE, CERT_SIZE),
	parser("")
{
	this->product = config->getAutomat()->createIterator();
}

Config4ConfigGenerator::~Config4ConfigGenerator() {
	delete this->product;
}

void Config4ConfigGenerator::reset() {
	state = State_Header;
	next();
}

void Config4ConfigGenerator::next()  {
	switch(state) {
	case State_Header: {
		generateHeader();
		state = State_Main;
		break;
	}
	case State_Main: generateMain(); break;
	case State_FiscalSection: generateFiscalSection(); break;
	case State_AuthPublicKey: generateAuthPublicKey(); break;
	case State_AuthPrivateKey: generateAuthPrivateKey(); break;
	case State_SignPrivateKey: generateSignPrivateKey(); break;
	case State_PriceLists: generatePriceLists(); break;
	case State_Products: generateProducts(); break;
	case State_Footer: {
		generateFooter();
		state = State_Complete;
		break;
	}
	case State_Complete: break;
	}
}

bool Config4ConfigGenerator::isLast() {
	return state == State_Complete;
}

/*
Example of configuration data:
DXS*XYZ1234567*VA*V0/6*1
ST*001*0001
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
IC1*EPHOR00001*135
// IC4 - описание валюты (обязателен)
// 1 - плавающая точка
// 2 - код страны
// 3 - описание валюты
IC4*2*0000
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
void Config4ConfigGenerator::generateMain() {
	Config1Boot *boot = config->getBoot();
	Config2Fiscal *fiscal = config->getFiscal();
	Config4Automat *automat = config->getAutomat();

	startBlock();
	*str << "IC1******" << automat->getAutomatId(); finishLine();
	*str << "IC4*" << automat->getDecimalPoint()
		 << "*" << automat->getCurrency()
		 << "**" << automat->getTaxSystem()
		 << "*" << automat->getMaxCredit()
		 << "*" << automat->getExt1Device()
		 << "*" << automat->getEvadts(); finishLine();
	*str << "AC2*" << boot->getImei()
		 << "*" << boot->getGprsApn()
		 << "*" << boot->getGprsUsername()
		 << "*" << boot->getGprsPassword()
		 << "*" << automat->getPaymentBus()
		 << "*" << boot->getHardwareVersion()
		 << "*" << boot->getFirmwareVersion()
		 << "*" << boot->getFirmwareRelease()
		 << "*" << boot->getGsmFirmwareVersion(); finishLine();
	*str << "AC3*" << fiscal->getKkt()
		 << "*" << fiscal->getKktInterface()
		 << "*" << fiscal->getKktAddr()
		 << "*" << fiscal->getKktPort()
		 << "*" << fiscal->getOfdAddr()
		 << "*" << fiscal->getOfdPort(); finishLine();
	finishBlock();

	state = State_FiscalSection;
}

void Config4ConfigGenerator::generateFiscalSection() {
	Config2Fiscal *fiscal = config->getFiscal();
	Config4Automat *automat = config->getAutomat();

	startBlock();
	*str << "FC2*" << fiscal->getINN() << "*" << automat->getAutomatId() << "*" << fiscal->getPointName() << "*" << fiscal->getPointAddr(); finishLine();
	finishBlock();

	gotoStateAuthPublicKey();
}

void Config4ConfigGenerator::gotoStateAuthPublicKey() {
	config->getFiscal()->getAuthPublicKey()->load(&cert);
	parser.init(cert.getString(), cert.getLen());
	state = State_AuthPublicKey;
}

void Config4ConfigGenerator::generateAuthPublicKey() {
	char buf[200];
	uint16_t len = parser.getValue("\r\n", buf, sizeof(buf));
	parser.skipEqual("\r\n\t ");
	startBlock();
	*str << "FC3*"; str->addStr(buf, len); finishLine();
	finishBlock();

	if(parser.hasUnparsed() == false) {
		gotoStateAuthPrivateKey();
	}
}

void Config4ConfigGenerator::gotoStateAuthPrivateKey() {
	config->getFiscal()->getAuthPrivateKey()->load(&cert);
	parser.init(cert.getString(), cert.getLen());
	state = State_AuthPrivateKey;
}

void Config4ConfigGenerator::generateAuthPrivateKey() {
	char buf[200];
	uint16_t len = parser.getValue("\r\n", buf, sizeof(buf));
	parser.skipEqual("\r\n\t ");
	startBlock();
	*str << "FC4*"; str->addStr(buf, len); finishLine();
	finishBlock();

	if(parser.hasUnparsed() == false) {
		gotoStateSignPrivateKey();
	}
}

void Config4ConfigGenerator::gotoStateSignPrivateKey() {
	config->getFiscal()->getSignPrivateKey()->load(&cert);
	parser.init(cert.getString(), cert.getLen());
	state = State_SignPrivateKey;
}

void Config4ConfigGenerator::generateSignPrivateKey() {
	char buf[200];
	uint16_t len = parser.getValue("\r\n", buf, sizeof(buf));
	parser.skipEqual("\r\n\t ");
	startBlock();
	*str << "FC5*"; str->addStr(buf, len); finishLine();
	finishBlock();

	if(parser.hasUnparsed() == false) {
		gotoStateGeneratePriceLists();
	}
}

void Config4ConfigGenerator::gotoStateGeneratePriceLists() {
	for(uint16_t i = 0; i < config->getAutomat()->getPriceListNum(); i++) {
		Config3PriceIndex *index = product->getPriceIdByIndex(i);
		if(index->type == Config3PriceIndexType_Time) {
			state = State_PriceLists;
			return;
		}
	}

	if(product->first() == false) {
		state = State_Footer;
		return;
	} else {
		state = State_Products;
		return;
	}
}

void Config4ConfigGenerator::generatePriceLists() {
	startBlock();
	for(uint16_t i = 0; i < config->getAutomat()->getPriceListNum(); i++) {
		Config3PriceIndex *index = product->getPriceIdByIndex(i);
		if(index->type == Config3PriceIndexType_Time) {
			WeekTable *week = index->timeTable.getWeek();
			TimeInterval *interval = index->timeTable.getInterval();
			*str << "LC2*" << index->device.get() << "*" << index->number << "*" << week->getValue()
				 << "*" << interval->getTime()->getHour() << ":" << interval->getTime()->getMinute() << ":" << interval->getTime()->getSecond()
				 << "*" << interval->getInterval(); finishLine();
		}
	}
	finishBlock();

	if(product->first() == false) {
		state = State_Footer;
		return;
	} else {
		state = State_Products;
		return;
	}
}

void Config4ConfigGenerator::generateProducts() {
	startBlock();
	*str << "PC1*" << product->getId() << "*" << product->getPrice("CA", 0)->getPrice() << "*" << product->getName(); finishLine();
	*str << "PC9*" << product->getId() << "*";
	if(product->getCashlessId() != Config3ProductIndexList::UndefinedIndex) { *str << product->getCashlessId(); }
	*str << "*" << product->getTaxRate() << "*";
	if(product->getWareId() > 0) { *str << product->getWareId(); }
	finishLine();
	generateProductPrices();
	finishBlock();

	if(product->next() == false) {
		state = State_Footer;
	}
}

void Config4ConfigGenerator::generateProductPrices() {
	for(uint16_t i = 0; i < config->getAutomat()->getPriceListNum(); i++) {
		Config3PriceIndex *index = product->getPriceIdByIndex(i);
		if(index->type == Config3PriceIndexType_None) {
			continue;
		}
		Config3Price *price = product->getPriceByIndex(i);
		*str << "PC7*" << product->getId();
		*str << "*" << index->device.get() << "*" << index->number << "*";
		*str << price->getPrice();
		finishLine();
	}
}
