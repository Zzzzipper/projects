#ifndef COMMON_MDB_MASTER_TESTCASHLESS_H_
#define COMMON_MDB_MASTER_TESTCASHLESS_H_

#include "MdbMasterCashless.h"

class TestMdbMasterCashless : public MdbMasterCashlessInterface {
public:
	TestMdbMasterCashless(uint16_t deviceId, StringBuilder *str);
	void setInited(bool inited) { this->inited = inited; }
	void setRefundAble(bool refundAble) { this->refundAble = refundAble; }
	void setResultRevalue(bool value) { this->resultRevalue = value; }
	void setResultSale(bool value) { this->resultSale = value; }
	void setResultSaleComplete(bool value) { this->resultSaleComplete = value; }
	void setResultSaleFailed(bool value) { this->resultSaleFailed = value; }
	void setResultCloseSession(bool value) { this->resultCloseSession = value; }

	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual void reset();
	virtual bool isRefundAble();
	virtual bool isInited();
	virtual void disable();
	virtual void enable();
	virtual bool revalue(uint32_t credit);
	virtual bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId);
	virtual bool saleComplete();
	virtual bool saleFailed();
	virtual bool closeSession();	

private:
	EventDeviceId deviceId;
	StringBuilder *str;
	uint16_t decimalPoint;
	bool inited;
	bool refundAble;
	bool resultRevalue;
	bool resultSale;
	bool resultSaleComplete;
	bool resultSaleFailed;
	bool resultCloseSession;
};

#endif
