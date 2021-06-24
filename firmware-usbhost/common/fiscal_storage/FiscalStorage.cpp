#include "include/FiscalStorage.h"

#include <string.h>

namespace FiscalStorage {

TaxSystem convertTaxSystem2FN105(uint8_t taxSystem) {
	switch(taxSystem) {
	case Fiscal::TaxSystem_OSN: return FiscalStorage::TaxSystem_OSN;
	case Fiscal::TaxSystem_USND: return FiscalStorage::TaxSystem_USND;
	case Fiscal::TaxSystem_USNDMR: return FiscalStorage::TaxSystem_USNDMR;
	case Fiscal::TaxSystem_ENVD: return FiscalStorage::TaxSystem_ENVD;
	case Fiscal::TaxSystem_ESN: return FiscalStorage::TaxSystem_ESN;
	case Fiscal::TaxSystem_Patent: return FiscalStorage::TaxSystem_Patent;
	default: return FiscalStorage::TaxSystem_OSN;
	}
}

TaxRate convertTaxRate2FN105(uint8_t taxRate) {
	switch(taxRate) {
	case Fiscal::TaxRate_NDSNone: return FiscalStorage::TaxRate_NDSNone;
	case Fiscal::TaxRate_NDS0: return FiscalStorage::TaxRate_NDS0;
	case Fiscal::TaxRate_NDS10: return FiscalStorage::TaxRate_NDS10;
	case Fiscal::TaxRate_NDS20: return FiscalStorage::TaxRate_NDS18;
	default: return FiscalStorage::TaxRate_NDS18;
	}
}

PaymentMethod convertPaymentMethod2FN105(uint8_t paymentMethod) {
	switch(paymentMethod) {
	case Fiscal::Payment_Cash: return FiscalStorage::PaymentMethod_Cash;
	case Fiscal::Payment_Cashless: return FiscalStorage::PaymentMethod_Cashless;
	default: return FiscalStorage::PaymentMethod_Cash;
	}
}

void addTlvHeader(FiscalStorage::Tag tag, uint16_t len, Buffer *buf) {
	FiscalStorage::Header tlv;
	tlv.tag.set(tag);
	tlv.len.set(len);
	buf->add(&tlv, sizeof(tlv));
}

void addTlvUint32(FiscalStorage::Tag tag, uint32_t value, Buffer *buf) {
	uint8_t v3 = value >> 24;
	uint8_t v2 = value >> 16;
	uint8_t v1 = value >> 8;
	uint8_t v0 = value;
	if(v3 > 0) {
		addTlvHeader(tag, 4, buf);
		buf->addUint8(v0);
		buf->addUint8(v1);
		buf->addUint8(v2);
		buf->addUint8(v3);
	} else if(v2 > 0) {
		addTlvHeader(tag, 3, buf);
		buf->addUint8(v0);
		buf->addUint8(v1);
		buf->addUint8(v2);
	} else if(v1 > 0) {
		addTlvHeader(tag, 2, buf);
		buf->addUint8(v0);
		buf->addUint8(v1);
	} else {
		addTlvHeader(tag, 1, buf);
		buf->addUint8(v0);
	}
}

void addTlvFUint32(FiscalStorage::Tag tag, uint8_t dotPos, uint8_t number, Buffer *buf) {
	addTlvHeader(tag, 2, buf);
	buf->addUint8(dotPos);
	buf->addUint8(number);
}

void addTlvString(FiscalStorage::Tag tag, const char *str, Buffer *buf) {
	uint16_t len = strlen(str);
	addTlvHeader(tag, len, buf);
	for(uint16_t i = 0; i < len; i++) {
		buf->addUint8(str[i]);
	}
}

}
