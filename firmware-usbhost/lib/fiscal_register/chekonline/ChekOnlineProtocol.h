#ifndef LIB_FISCALREGISTER_CHEKONLINE_PROTOCOL_H
#define LIB_FISCALREGISTER_CHEKONLINE_PROTOCOL_H

#define CHEKONLINE_MANUFACTURER	"CHK"
#define CHEKONLINE_MODEL		"Chekonline"

namespace ChekOnline {

enum TaxRate {
	TaxRate_NDSNone = 4,
	TaxRate_NDS0	= 3,
	TaxRate_NDS10	= 2,
	TaxRate_NDS20	= 1,
};

extern TaxRate convertTaxRate2ChekOnline(uint8_t taxRate);

}

#endif
