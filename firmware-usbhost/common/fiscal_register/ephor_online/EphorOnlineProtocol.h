#ifndef LIB_FISCALREGISTER_EPHORONLINE_PROTOCOL_H
#define LIB_FISCALREGISTER_EPHORONLINE_PROTOCOL_H

#define CHEKONLINE_MANUFACTURER	"CHK"
#define CHEKONLINE_MODEL		"Chekonline"

namespace EphorOnline {

enum TaxRate {
	TaxRate_NDSNone = 4,
	TaxRate_NDS0	= 3,
	TaxRate_NDS10	= 2,
	TaxRate_NDS18	= 1,
};

extern TaxRate convertTaxRate2ChekOnline(uint8_t taxRate);

}

#endif
