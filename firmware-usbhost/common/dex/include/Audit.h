#ifndef COMMON_DEX_AUDIT_H_
#define COMMON_DEX_AUDIT_H_

#include "AuditProduct.h"

#include "utils/include/List.h"

class Audit {
public:
	Audit();
	virtual ~Audit();
	void setAutomatId(uint32_t automatId);
	uint32_t getAutomatId() { return automatId; }
	void setDecimalPlaces(uint16_t decimalPoint);
	uint16_t getDecimalPoint() { return decimalPoint; }
	uint32_t convertValueToMoney(uint32_t value) const;
	uint32_t convertMoneyToValue(uint32_t value) const;
	uint16_t len();
	void addProduct(AuditProduct *product);
	AuditProduct *getProductById(const char *id);
	AuditProduct *getProductByIndex(uint16_t index);
	AuditProduct *search(Audit *audit1);
	void clear();

private:
	uint32_t automatId;
	uint16_t decimalPoint;
	uint32_t devider;
	List<AuditProduct> products;

	void calcDevider(uint16_t decimalPoint);
};

#endif
