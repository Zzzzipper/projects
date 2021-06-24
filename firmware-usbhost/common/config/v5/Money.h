#ifndef COMMON_CONFIG_V5_MONEY_H_
#define COMMON_CONFIG_V5_MONEY_H_

#include "fiscal_register/include/FiscalSale.h"
#include "utils/include/List.h"

#include <stdint.h>

namespace Money {

class Payment {
public:
	Payment();
	void set(uint8_t method, uint8_t type, uint32_t value);
	uint8_t getMethod();
	uint8_t getType();
	uint32_t getValue();

private:
	uint8_t method;
	uint8_t type;
	uint32_t value;
};

class Sum {
public:
	Sum();
	void setCurrency(uint16_t currency);
	void add(uint8_t method, uint8_t type, uint32_t value);
	uint16_t getCurrency();
	uint32_t getSum();
	uint32_t getPaymentNum();
	Payment *getPayment(uint16_t index);

private:
	uint16_t currency;
	uint8_t paymentNum;
	List<Payment> payment;
};

}

#endif
