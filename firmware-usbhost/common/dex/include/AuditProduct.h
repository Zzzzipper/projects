#ifndef COMMON_DEX_AUDITPRODUCT_H_
#define COMMON_DEX_AUDITPRODUCT_H_

#include "utils/include/StringBuilder.h"

class AuditProduct {
public:
	StringBuilder id;
	StringBuilder name;
	uint32_t ca0price;
	uint32_t ca0saleTotal;

	AuditProduct();
	virtual ~AuditProduct();
	bool compareId(const char *id);

private:
};

#endif
