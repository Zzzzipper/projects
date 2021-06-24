#include "TestMdbMasterCashless.h"

TestMdbMasterCashless::TestMdbMasterCashless(uint16_t deviceId, StringBuilder *str) :
	deviceId(deviceId),
	str(str),
	inited(false),
	refundAble(false),
	resultRevalue(true),
	resultSale(true),
	resultSaleComplete(true),
	resultSaleFailed(true),
	resultCloseSession(true)
{

}

void TestMdbMasterCashless::reset() {
	*str << "<MCL(" <<  deviceId.getValue() << ")::reset>";
}

bool TestMdbMasterCashless::isRefundAble() {
	return refundAble;
}

bool TestMdbMasterCashless::isInited() {
	return inited;
}

void TestMdbMasterCashless::disable() {
	*str << "<MCL(" <<  deviceId.getValue() << ")::disable>";
}

void TestMdbMasterCashless::enable() {
	*str << "<MCL(" <<  deviceId.getValue() << ")::enable>";
}

bool TestMdbMasterCashless::revalue(uint32_t credit) {
	*str << "<MCL(" <<  deviceId.getValue() << ")::revalue(" << credit << ")>";
	return resultRevalue;
}

bool TestMdbMasterCashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	(void)productName;
	(void)wareId;
	*str << "<MCL(" <<  deviceId.getValue() << ")::sale(" << productId << "," << productPrice << ")>";
	return resultSale;
}

bool TestMdbMasterCashless::saleComplete() {
	*str << "<MCL(" <<  deviceId.getValue() << ")::saleComplete>";
	return resultSaleComplete;
}

bool TestMdbMasterCashless::saleFailed() {
	*str << "<MCL(" <<  deviceId.getValue() << ")::saleFailed>";
	return resultSaleFailed;
}

bool TestMdbMasterCashless::closeSession() {
	*str << "<MCL(" <<  deviceId.getValue() << ")::closeSession>";
	return resultCloseSession;
}
