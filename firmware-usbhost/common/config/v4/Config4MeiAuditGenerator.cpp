#include "Config4MeiAuditGenerator.h"
#include "dex/DexProtocol.h"
#include "logger/include/Logger.h"

Config4MeiAuditGenerator::Config4MeiAuditGenerator(Config4Modem *config) :
	EvadtsGenerator("9252131001"),
	config(config)
{
	this->product = config->getAutomat()->createIterator();
}

Config4MeiAuditGenerator::~Config4MeiAuditGenerator() {
	delete this->product;
}

void Config4MeiAuditGenerator::reset() {
	state = State_Header;
	next();
}

void Config4MeiAuditGenerator::next()  {
	switch(state) {
		case State_Header: {
			generateHeader();
			state = State_CoinChanger;
			break;
		}
		case State_CoinChanger: generateCoinChanger(); break;
		case State_Main: generateMain(); break;
		case State_PriceList0: generatePriceList0(); break;
		case State_PriceList1: generatePriceList1(); break;
		case State_Products: generateProducts(); break;
		case State_Total: generateTotal(); break;
		case State_Footer: {
			generateFooter();
			state = State_Complete;
			break;
		}
		case State_Complete: break;
	}
}

bool Config4MeiAuditGenerator::isLast() {
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
void Config4MeiAuditGenerator::generateCoinChanger() {
	startBlock();
	Config4ProductListStat *saleStat = config->getAutomat()->getProductList()->getStat();
	*str << "CA2*" << saleStat->data.vend_cash_val_total << "*" << saleStat->data.vend_cash_num_total
		 << "*" << saleStat->data.vend_cash_val_reset << "*" << saleStat->data.vend_cash_num_reset; finishLine();

	Config4PaymentStat *paymentStat = config->getAutomat()->getPaymentStat();
	*str << "CA3*" << paymentStat->data.cash_in_reset << "*" << paymentStat->data.coin_in_cashbox_reset
		 << "*" << paymentStat->data.coin_in_tubes_reset << "*" << paymentStat->data.bill_in_reset/100
		 << "*" << paymentStat->data.cash_in_total << "*" << paymentStat->data.coin_in_cashbox_total
		 << "*" << paymentStat->data.coin_in_tubes_total << "*" << paymentStat->data.bill_in_total/100
		 << "*" << paymentStat->data.bill_in_reset << "*" << paymentStat->data.bill_in_total; finishLine();
	*str << "CA4*" << paymentStat->data.coin_dispense_reset << "*" << paymentStat->data.coin_manually_dispense_reset
		 << "*" << paymentStat->data.coin_dispense_total << "*" << paymentStat->data.coin_manually_dispense_total; finishLine();
	*str << "CA8*0*0"; finishLine();
	*str << "CA9*0*0"; finishLine();
	*str << "CA10*" << paymentStat->data.coin_filled_reset << "*" << paymentStat->data.coin_filled_total; finishLine();

	MdbCoinChangerContext *context = config->getAutomat()->getCCContext();
	*str << "CA15*" << context->getInTubeValue(); finishLine();
	for(uint16_t i = 0; i < context->getSize(); i++) {
		MdbCoin *coin = context->get(i);
		if(coin->getInTube() == true) {
			*str << "CA17*" << i << "*" << coin->getNominal() << "*" << coin->getNumber() << "*0*0*" << (coin->getFullTube() == true ? 1 : 0); finishLine();
		}
	}
	finishBlock();
	state = State_Main;
}

void Config4MeiAuditGenerator::generateMain() {
	Config4Automat *automat = config->getAutomat();
	startBlock();
	*str << "ID1******" << automat->getAutomatId(); finishLine();
	*str << "ID4*" << automat->getDecimalPoint() << "*" << automat->getCurrency(); finishLine();
	finishBlock();

	if(product->first() == false) {
		state = State_Footer;
	} else {
		gotoStatePriceList0();
	}
}

void Config4MeiAuditGenerator::gotoStatePriceList0() {
	product->first();
	state = State_PriceList0;
}

void Config4MeiAuditGenerator::generatePriceList0() {
	startBlock();
	Config3Price *price = product->getPrice("CA", 0);
	if(price == NULL) {
		return;
	}
	*str << "LA1*0*" << product->getId() << "*" << price->getPrice() << "*" << price->getCount() << "*" << price->getTotalCount(); finishLine();
	finishBlock();

	if(product->next() == false) {
		gotoStatePriceList1();
	}
}

void Config4MeiAuditGenerator::gotoStatePriceList1() {
	product->first();
	state = State_PriceList1;
}

void Config4MeiAuditGenerator::generatePriceList1() {
	startBlock();
	Config3Price *price = product->getPrice("DA", 1);
	if(price == NULL) {
		return;
	}
	*str << "LA1*1*" << product->getId() << "*" << price->getPrice() << "*" << price->getCount() << "*" << price->getTotalCount(); finishLine();
	finishBlock();

	if(product->next() == false) {
		gotoStateProducts();
	}
}

void Config4MeiAuditGenerator::gotoStateProducts() {
	product->first();
	state = State_Products;
}

void Config4MeiAuditGenerator::generateProducts() {
	startBlock();
	*str << "PA1*" << product->getId() << "*" << product->getPrice("CA", 0)->getPrice(); finishLine();
	*str << "PA2*" << product->getTotalCount() << "*" << product->getTotalMoney()
		 << "*" << product->getCount() << "*" << product->getMoney(); finishLine();
	*str << "PA4*" << product->getFreeTotalCount() << "*0*" << product->getFreeCount() << "*0"; finishLine();
	finishBlock();

	if(product->next() == false) {
		gotoStateTotal();
	}
}

void Config4MeiAuditGenerator::gotoStateTotal() {
	state = State_Total;
}

void Config4MeiAuditGenerator::generateTotal() {
	Config4ProductListStat *saleStat = config->getAutomat()->getProductList()->getStat();
	uint32_t vend_val_total = saleStat->data.vend_token_val_total + saleStat->data.vend_cash_val_total + saleStat->data.vend_cashless1_val_total + saleStat->data.vend_cashless2_val_total;
	uint32_t vend_num_total = saleStat->data.vend_token_num_total + saleStat->data.vend_cash_num_total + saleStat->data.vend_cashless1_num_total + saleStat->data.vend_cashless2_num_total;
	uint32_t vend_val_reset = saleStat->data.vend_token_val_reset + saleStat->data.vend_cash_val_reset + saleStat->data.vend_cashless1_val_reset + saleStat->data.vend_cashless2_val_reset;
	uint32_t vend_num_reset = saleStat->data.vend_token_num_reset + saleStat->data.vend_cash_num_reset + saleStat->data.vend_cashless1_num_reset + saleStat->data.vend_cashless2_num_reset;
	startBlock();
	*str << "VA1*" << vend_val_total << "*" << vend_num_total
		 << "*" << vend_val_reset << "*" << vend_num_reset; finishLine();
	*str << "VA3*0*0*0*0"; finishLine();
	finishBlock();
	state = State_Footer;
}
