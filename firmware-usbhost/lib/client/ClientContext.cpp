#include "ClientContext.h"

#include "common/logger/include/Logger.h"

#define NAME_SIZE 64
#define LOYALITY_CODE_SIZE 128

ClientContext::ClientContext() :
	locked(false),
	firstName(NAME_SIZE, NAME_SIZE),
	lastName(NAME_SIZE, NAME_SIZE),
	loyalityType(Loyality_None),
	loyalityCode(LOYALITY_CODE_SIZE),
	discount(0)
{

}

void ClientContext::setFirstName(const char *name) {
	firstName.set(name);
}

const char *ClientContext::getFirstName() {
	return firstName.getString();
}

StringBuilder *ClientContext::getFirstNameStr() {
	return &firstName;
}

void ClientContext::setLastName(const char *name) {
	lastName.set(name);
}

const char *ClientContext::getLastName() {
	return lastName.getString();
}

StringBuilder *ClientContext::getLastNameStr() {
	return &lastName;
}

void ClientContext::setLoyality(uint8_t loyalityType, const uint8_t *loyalityData, uint32_t loyalityLen) {
	this->loyalityType = loyalityType;
	this->loyalityCode.clear();
	this->loyalityCode.add(loyalityData, loyalityLen);
}

uint8_t ClientContext::getLoyalityType() {
	return loyalityType;
}

const uint8_t *ClientContext::getLoyalityCode() {
	return loyalityCode.getData();
}

uint16_t ClientContext::getLoyalityLen() {
	return loyalityCode.getLen();
}

void ClientContext::setDiscount(uint32_t discount) {
	if(discount > 100) { this->discount = 100; } else { this->discount = discount; }
}

uint32_t ClientContext::getDiscount() {
	return discount;
}

uint32_t ClientContext::calcDiscount(uint32_t price) {
	return price * (100 - discount) / 100;
}

bool ClientContext::reset() {
	if(locked == true) { return false; }
	firstName.clear();
	lastName.clear();
	loyalityType = Loyality_None;
	loyalityCode.clear();
	discount = 0;
	return true;
}

void ClientContext::startVending() {
	locked = true;
}

void ClientContext::completeVending() {
	locked = false;
	reset();
}
