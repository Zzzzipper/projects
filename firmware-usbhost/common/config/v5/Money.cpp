#include "config/v5/Money.h"

namespace Money {

Payment::Payment() :
	method(0),
	type(0),
	value(0)
{

}

void Payment::set(uint8_t method, uint8_t type, uint32_t value) {
	this->method = method;
	this->type = type;
	this->value = value;
}

uint8_t Payment::getMethod() {
	return method;
}

uint8_t Payment::getType() {
	return type;
}

uint32_t Payment::getValue() {
	return value;
}

Sum::Sum() :
	currency(0)
{

}

void Sum::setCurrency(uint16_t currency) {
	this->currency = currency;
}

void Sum::add(uint8_t method, uint8_t type, uint32_t value) {

}

uint16_t Sum::getCurrency() {
	return currency;
}

uint32_t Sum::getSum() {
	return 0;
}

uint32_t Sum::getPaymentNum() {
	return 0;
}

Payment *Sum::getPayment(uint16_t index) {
	return 0;
}

}
