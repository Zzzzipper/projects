#include "include/FiscalRegister.h"
#include "utils/include/DecimalPoint.h"
#include "logger/include/Logger.h"

namespace Fiscal {

Product::Product() :
	wareId(0),
	price(0),
	taxRate(0),
	quantity(0),
	discountType(0),
	discountValue(0)
{
}

void Product::set(const char *selestId, uint32_t wareId, const char *name, uint32_t price, uint8_t taxRate, uint8_t quantity) {
	this->selectId.set(selestId);
	this->wareId = wareId;
	this->name.set(name);
	this->price = price;
	this->taxRate = taxRate;
	this->quantity = quantity;
	this->discountType = 0;
	this->discountValue = 0;
}

uint32_t Product::getTaxValue() {
	switch(taxRate) {
	case Fiscal::TaxRate_NDSNone: return 0;
	case Fiscal::TaxRate_NDS0: return 0;
	case Fiscal::TaxRate_NDS10: return (price*10/110);
	case Fiscal::TaxRate_NDS20: return (price*20/120);
	default: return 0;
	}
}

ProductList::ProductList() :
	list(FISCAL_PRODUCT_NUM)
{
}

void ProductList::set(ProductList *list) {
	this->list.clear();
	for(uint16_t i = 0; i < list->list.getSize(); i++) {
		Product *product = list->list.get(i);
		this->list.add(product);
	}
}

void ProductList::set(const char *selectId, uint32_t wareId, const char *name, uint32_t price, uint8_t taxRate, uint8_t quantity) {
	uint16_t num = list.getSize();
	if(num == 0) {
		add(selectId, wareId, name, price, taxRate, quantity);
		return;
	} else if(num == 1) {
		Product *product = list.get(0);
		product->set(selectId, wareId, name, price, taxRate, quantity);
		return;
	} else {
		clear();
		add(selectId, wareId, name, price, taxRate, quantity);
		return;
	}
}

void ProductList::add(const char *selectId, uint32_t wareId, const char *name, uint32_t price, uint8_t taxRate, uint8_t quantity) {
	Product *product = new Product;
	product->set(selectId, wareId, name, price, taxRate, quantity);
	list.add(product);
}

void ProductList::clear() {
	list.clearAndFree();
}

uint16_t ProductList::getNum() {
	return list.getSize();
}

Product *ProductList::get(uint16_t index) {
	return list.get(index);
}

uint32_t ProductList::getPrice() {
	uint32_t price = 0;
	for(uint16_t i = 0; i < this->list.getSize(); i++) {
		Product *product = this->list.get(i);
		price += product->price * product->quantity;
	}
	return price;
}

uint32_t ProductList::getTaxValue() {
	uint32_t tax = 0;
	for(uint16_t i = 0; i < this->list.getSize(); i++) {
		Product *product = this->list.get(i);
		tax += product->getTaxValue();
	}
	return tax;
}

Sale::Sale() :
	priceList(0),
	paymentType(0),
	credit(0),
	taxSystem(0),
	taxValue(0),
	loyalityType(0),
	fiscalRegister(0),
	fiscalStorage(0),
	fiscalDocument(0),
	fiscalSign(0)
{
}

void Sale::set(Sale *sale) {
#if 0
	datetime = sale->datetime;
	selectId.set(sale->selectId.get());
	wareId = sale->wareId;
	name.set(sale->name.get());
	device.set(sale->device.get());
	priceList = sale->priceList;
	price = sale->price;
	paymentType = sale->paymentType;
	credit = sale->credit;
	taxSystem = sale->taxSystem;
	taxRate = sale->taxRate;
	taxValue = sale->taxValue;
	loyalityType = sale->loyalityType;
	loyalityCode.set(sale->loyalityCode.getData(), sale->loyalityCode.getLen());
	fiscalDatetime = sale->fiscalDatetime;
	fiscalRegister = sale->fiscalRegister;
	fiscalStorage = sale->fiscalStorage;
	fiscalDocument = sale->fiscalDocument;
	fiscalSign = sale->fiscalSign;
#else
	datetime = sale->datetime;
	products.set(&sale->products);
	device.set(sale->device.get());
	priceList = sale->priceList;
	paymentType = sale->paymentType;
	credit = sale->credit;
	taxSystem = sale->taxSystem;
	taxValue = sale->taxValue;
	loyalityType = sale->loyalityType;
	loyalityCode.set(sale->loyalityCode.getData(), sale->loyalityCode.getLen());
	fiscalDatetime = sale->fiscalDatetime;
	fiscalRegister = sale->fiscalRegister;
	fiscalStorage = sale->fiscalStorage;
	fiscalDocument = sale->fiscalDocument;
	fiscalSign = sale->fiscalSign;
#endif
}

void Sale::setProduct(const char *selectId, uint32_t wareId, const char *name, uint32_t price, uint8_t taxRate, uint8_t quantity) {
	products.set(selectId, wareId, name, price, taxRate, quantity);
}

void Sale::addProduct(const char *selectId, uint32_t wareId, const char *name, uint32_t price, uint8_t taxRate, uint8_t quantity) {
	products.add(selectId, wareId, name, price, taxRate, quantity);
}

void Sale::clearProducts() {
	products.clear();
}

uint16_t Sale::getProductNum() {
	return products.getNum();
}

Product *Sale::getProduct(uint16_t index) {
	return products.get(index);
}

uint32_t Sale::getPrice() {
	return products.getPrice();
}

enum FiscalEventErrorParam {
	FiscalEventErrorParam_Code = 0,
	FiscalEventErrorParam_Data,
};

EventError::EventError() :
	EventInterface(Register::Event_CommandError),
	code(ConfigEvent::Type_None),
	data(FISCAL_ERROR_DATA_SIZE, FISCAL_ERROR_DATA_SIZE)
{

}

EventError::EventError(EventDeviceId deviceId) :
	EventInterface(deviceId, Register::Event_CommandError),
	code(ConfigEvent::Type_None),
	data(FISCAL_ERROR_DATA_SIZE, FISCAL_ERROR_DATA_SIZE)
{
}

bool EventError::open(EventEnvelope *envelope) {
	if(envelope->getType() != type) { return false; }
	device.setValue(envelope->getDevice());
	if(envelope->getUint16(FiscalEventErrorParam_Code, &code) == false) { return false; }
	if(envelope->getString(FiscalEventErrorParam_Data, &data) == false) { return false; }
	return true;
}

bool EventError::pack(EventEnvelope *envelope) {
	envelope->clear();
	envelope->setType(type);
	envelope->setDevice(device.getValue());
	if(envelope->addUint16(FiscalEventErrorParam_Code, code) == false) { return false; }
	if(envelope->addString(FiscalEventErrorParam_Data, data.getString()) == false) { return false; }
	return true;
}

DummyRegister::DummyRegister(EventEngineInterface *eventEngine) :
	eventEngine(eventEngine),
	deviceId(eventEngine)
{

}

EventDeviceId DummyRegister::getDeviceId() {
	return deviceId;
}

void DummyRegister::reset() {

}

void DummyRegister::sale(Sale *saleData, uint32_t decimalPoint) {
#if 0
	uint32_t price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->price);
	saleData->taxValue = calcTax(saleData->taxRate, price);
	saleData->fiscalRegister = 0;
	saleData->fiscalStorage = 0;
	saleData->fiscalDocument = 0;
	saleData->fiscalSign = 0;
	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	eventEngine->transmit(&event);
#else
	Fiscal::Product *product = saleData->getProduct(0);
	uint32_t price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, product->price);
	saleData->taxValue = calcTax(product->taxRate, price);
	saleData->fiscalRegister = 0;
	saleData->fiscalStorage = 0;
	saleData->fiscalDocument = 0;
	saleData->fiscalSign = 0;
	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	eventEngine->transmit(&event);
#endif
}

void DummyRegister::getLastSale() {

}

void DummyRegister::closeShift() {

}

uint32_t DummyRegister::calcTax(uint8_t taxRate, uint32_t profit) {
	switch(taxRate) {
	case Fiscal::TaxRate_NDSNone: return 0;
	case Fiscal::TaxRate_NDS0: return 0;
	case Fiscal::TaxRate_NDS10: return (profit*10/110);
	case Fiscal::TaxRate_NDS20: return (profit*20/120);
	default: return 0;
	}
}

Context::Context(uint32_t masterDecimalPoint, RealTimeInterface *realtime) :
	Mdb::DeviceContext(masterDecimalPoint, realtime)
{
	converter.setDeviceDecimalPoint(FISCAL_DECIMAL_POINT);
}

uint32_t Context::calcTax(uint8_t taxRate, uint32_t profit) {
	switch(taxRate) {
	case Fiscal::TaxRate_NDSNone: return 0;
	case Fiscal::TaxRate_NDS0: return 0;
	case Fiscal::TaxRate_NDS10: return (profit*10/110);
	case Fiscal::TaxRate_NDS20: return (profit*20/120);
	default: return 0;
	}
}

}
