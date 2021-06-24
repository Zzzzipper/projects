#ifndef LIB_CLIENT_CONTEXT_H_
#define LIB_CLIENT_CONTEXT_H_

#include "common/utils/include/Buffer.h"
#include "common/utils/include/StringBuilder.h"

enum Loyality {
	Loyality_None	 = 0,
	Loyality_Nefm	 = 1,
	Loyality_Mifare	 = 2,
};

class ClientContext {
public:
	ClientContext();
	void setFirstName(const char *name);
	const char *getFirstName();
	StringBuilder *getFirstNameStr();
	void setLastName(const char *name);
	const char *getLastName();
	StringBuilder *getLastNameStr();
	void setLoyality(uint8_t loyalityType, const uint8_t *loyalityData, uint32_t loyalityLen);
	uint8_t getLoyalityType();
	const uint8_t *getLoyalityCode();
	uint16_t getLoyalityLen();
	void setDiscount(uint32_t discount);
	uint32_t getDiscount();
	uint32_t calcDiscount(uint32_t price);

	bool reset();
	void startVending();
	void completeVending();

private:
	bool locked;
	StringBuilder firstName;
	StringBuilder lastName;
	uint8_t loyalityType;
	Buffer  loyalityCode;
	uint32_t discount;
};

#endif
