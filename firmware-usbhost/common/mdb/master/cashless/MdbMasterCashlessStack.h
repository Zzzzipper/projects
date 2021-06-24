#ifndef COMMON_MDB_MASTER_CASHLESS_STACK_H_
#define COMMON_MDB_MASTER_CASHLESS_STACK_H_

#include "MdbMasterCashlessInterface.h"

#include "common/utils/include/List.h"

class MdbMasterCashlessStack {
public:
	void push(MdbMasterCashlessInterface *masterCashless);
	void reset();
	void enable();
	void disable();
	void disableNotRefundAble();
	bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId);
	void closeSession();
	MdbMasterCashlessInterface *find(uint16_t deviceId);

private:
	List<MdbMasterCashlessInterface> list;
};

#endif
