#include "include/Audit.h"

#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

#define DECIMAL_PLACES_MIN 0
#define DECIMAL_PLACES_MAX 2
#define DECIMAL_PLACES_DEFAULT 2

Audit::Audit() : automatId(0), products(20) {
	setDecimalPlaces(DECIMAL_PLACES_DEFAULT);
}

Audit::~Audit() {
	products.clearAndFree();
}

void Audit::setAutomatId(uint32_t automatId) {
	this->automatId = automatId;
}

void Audit::setDecimalPlaces(uint16_t decimalPoint) {
	if(decimalPoint > DECIMAL_PLACES_MAX) {
		return;
	}
	this->decimalPoint = decimalPoint;
	calcDevider(decimalPoint);
}

void Audit::calcDevider(uint16_t decimalPoint) {
	devider = 1;
	for(uint16_t i = 0; i < decimalPoint; i++) {
		devider = devider * 10;
	}
}

uint32_t Audit::convertValueToMoney(uint32_t value) const {
	return (value / devider);
}

uint32_t Audit::convertMoneyToValue(uint32_t money) const {
	return (money * devider);
}

uint16_t Audit::len() {
	return products.getSize();
}

void Audit::addProduct(AuditProduct *product) {
	products.add(product);
}

AuditProduct *Audit::getProductById(const char *id) {
	for(uint16_t i = 0; i < products.getSize(); i++) {
		AuditProduct *product = products.get(i);
		if(product->compareId(id) == true) {
			return product;
		}
	}
	return NULL;
}

AuditProduct *Audit::getProductByIndex(uint16_t index) {
	if(index >= products.getSize()) {
		LOG_ERROR(LOG_EVADTS, "Index is out of range " << index);
		return NULL;
	}
	return products.get(index);
}

AuditProduct *Audit::search(Audit *audit1) {
	if(products.getSize() != audit1->products.getSize()) {
		LOG_ERROR(LOG_EVADTS, "Different product number");
		return NULL;
	}
	for(uint16_t i = 0; i < products.getSize(); i++) {
		AuditProduct *p1 = products.get(i);
		AuditProduct *p2 = audit1->getProductById(p1->id.getString());
		if(p2 == NULL) {
			LOG_ERROR(LOG_EVADTS, "Products not equal");
			return NULL;
		}
		if(p1->ca0saleTotal != p2->ca0saleTotal) {
			return p1;
		}
	}
	return NULL;
}

void Audit::clear() {
	products.clearAndFree();
}
