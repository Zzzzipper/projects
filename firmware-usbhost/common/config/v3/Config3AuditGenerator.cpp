#include "Config3AuditGenerator.h"
#include "dex/DexProtocol.h"
#include "logger/include/Logger.h"

Config3AuditGenerator::Config3AuditGenerator(Config3Modem *config) :
	EvadtsGenerator(config->getAutomat()->getCommunictionId()),
	config(config)
{
	this->product = config->getAutomat()->createIterator();
}

Config3AuditGenerator::~Config3AuditGenerator() {
	delete this->product;
}

void Config3AuditGenerator::reset() {
	state = State_Header;
	next();
}

void Config3AuditGenerator::next()  {
	switch(state) {
		case State_Header: {
			generateHeader();
			state = State_Main;
			break;
		}
		case State_Main: generateMain(); break;
		case State_CoinChanger: generateCoinChanger(); break;
		case State_Products: generateProducts(); break;
		case State_ProductPrices: generateProductPrices(); break;
		case State_Footer: {
			generateFooter();
			state = State_Complete;
			break;
		}
		case State_Complete: break;
	}
}

bool Config3AuditGenerator::isLast() {
	return state == State_Complete;
}

/*
DXS*XYZ1234567*VA*V0/6*1
ST*001*0001
// AM2 - параметры телеметрии
// 1 - IMEI (изменять нельзя)
// 2 - GPRS APN
// 3 - GPRS логин
// 4 - GPRS пароль
// 5 - Платежная шина (MDB/EXECUTIVE)
// 6 - Firmware version
// 7 - Firmware autoupdate
// 8 - GSM Firmware version
AM2*123456789012345*internet*gdata*gdata*1
// AM3 - параметры фискального регистратора
// 1 - идентификатор ККТ
// 2 - интерфейс ККТ
// 3 - IP-адрес ККТ
// 4 - порт ККТ
// 5 - IP-адрес ОФД
// 6 - порт ОФД
AM3*1*1*192.168.1.201*5555*91.107.67.212*7790
// ID1 - описание автомата
// 1 - серийный номер
// 6 - номер ТА
ID1*EPHOR00001*135
// IC4 - описание валюты (обязателен)
// 1 - плавающая точка
// 2 - код страны
// 3 - описание валюты
ID4*2*0000
// PA1 - описание продукта
// 1 - идентификатор продукта (ячейки)
// 2 - цена продукта
// 3 - идентификатор товара
// 4 - максимальная загрузка товара
// 5 - Standard Filling Level
// 6 - Standard Dispensed Quantity
// 7 - Selection status (0 - включена, 1 - отключена)
PA1*10******1
PA1*11**Чипсы****0
// PA7 - цена продукта
// 1 - идентификатор продукта (ячейки)
// 2 - способ оплаты (CA - наличные, DA - картридер 1, DB - картридер 2)
// 3 - номер прайс листа в способе оплаты
// 4 - значение цены
PA7*11*CA*0*3500
PA7*11*DA*1*3500
PA7*11*DA*2*3500
PA7*11*DA*3*3500
G85*1234 (example G85 CRC is 1234)
SE*9*0001
DXE*1*1
*/
void Config3AuditGenerator::generateMain() {
	Config1Boot *boot = config->getBoot();
	Config2Fiscal *fiscal = config->getFiscal();
	Config3Automat *automat = config->getAutomat();

	startBlock();
	*str << "AM2*" << boot->getImei()
		 << "*" << boot->getGprsApn()
		 << "*" << boot->getGprsUsername()
		 << "*" << boot->getGprsPassword()
		 << "*" << automat->getPaymentBus()
		 << "*" << boot->getHardwareVersion()
		 << "*" << boot->getFirmwareVersion()
		 << "*1"
		 << "*" << boot->getGsmFirmwareVersion(); finishLine();
	*str << "AM3*" << fiscal->getKkt()
		 << "*" << fiscal->getKktInterface()
		 << "*" << fiscal->getKktAddr()
		 << "*" << fiscal->getKktPort()
		 << "*" << fiscal->getOfdAddr()
		 << "*" << fiscal->getOfdPort(); finishLine();
	*str << "ID1******" << automat->getAutomatId(); finishLine();
	*str << "ID4*" << automat->getDecimalPoint() << "*7"; finishLine();
	finishBlock();

	state = State_CoinChanger;
}

void Config3AuditGenerator::generateCoinChanger() {
	MdbCoinChangerContext *context = config->getAutomat()->getCCContext();
	startBlock();
	*str << "CA15*" << context->getInTubeValue(); finishLine();
	for(uint16_t i = 0; i < context->getSize(); i++) {
		MdbCoin *coin = context->get(i);
		if(coin->getInTube() == true) {
			*str << "CA17*" << i << "*" << coin->getNominal() << "*" << coin->getNumber() << "***" << (coin->getFullTube() == true ? 1 : 0); finishLine();
		}
	}
	finishBlock();

	if(product->first() == false) {
		state = State_Footer;
	} else {
		state = State_Products;
	}
}

void Config3AuditGenerator::generateProducts() {
	startBlock();
#if 1
	*str << "PA1*" << product->getId() << "*" << product->getPrice("CA", 0)->getPrice() << "*"; win1251ToLatin(product->getName()); finishLine();
#else
	*str << "PA1*" << product->getId() << "*" << product->getPrice("CA", 0)->getPrice() << "*" << product->getName(); finishLine();
#endif

	*str << "PA9*" << product->getId() << "*";
	if(product->getCashlessId() != Config3ProductIndexList::UndefinedIndex) { *str << product->getCashlessId(); }
	*str << "*" << product->getTaxRate();
	if(product->getWareId() != 0) { *str << "*" << product->getWareId(); }
	finishLine();

	*str << "PA4*" << product->getFreeTotalCount(); finishLine();
	finishBlock();

	priceListIndex = 0;
	state = State_ProductPrices;
}

void Config3AuditGenerator::generateProductPrices() {
	uint16_t priceListIndexMax = priceListIndex + 3;
	uint16_t priceListIndexNum = config->getAutomat()->getPriceListNum();
	if(priceListIndexMax > priceListIndexNum) {
		priceListIndexMax = priceListIndexNum;
	}

	startBlock();
	for(; priceListIndex < priceListIndexMax; priceListIndex++) {
		Config3PriceIndex *index = product->getPriceIdByIndex(priceListIndex);
		if(index->type == Config3PriceIndexType_None) {
			continue;
		}
		Config3Price *price = product->getPriceByIndex(priceListIndex);
		*str << "PA7*" << product->getId();
		*str << "*" << index->device.get() << "*" << index->number << "*";
		*str << price->getPrice();
		*str << "*" << price->getTotalCount();
		if(price->getTotalMoney() != EvadtsUint32Undefined) {
			*str << "*" << price->getTotalMoney();
		}
		finishLine();
	}
	finishBlock();

	if(priceListIndex >= priceListIndexNum) {
		if(product->next() == true) {
			state = State_Products;
		} else {
			state = State_Footer;
		}
	}
}
