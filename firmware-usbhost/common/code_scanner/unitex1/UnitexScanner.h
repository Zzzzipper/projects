#ifndef LIB_SALEMANAGER_CCI_UNITEXSCANNER_H_
#define LIB_SALEMANAGER_CCI_UNITEXSCANNER_H_

#include "common/mdb/master/cashless/MdbMasterCashless.h"
#include "common/code_scanner/CodeScanner.h"
#include "common/utils/include/Fifo.h"

class UnitexSale {
public:
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint32_t checkNum;
	uint32_t cashlessId;

	void clear();

	friend bool operator==(const UnitexSale &s1, const UnitexSale &s2);
};

#if 0
class UnitexSales {
public:
	UnitexSales();
	void clear();
	bool push(UnitexSale saleNum);
	bool isUsed(UnitexSale saleNum);

private:
	Fifo<UnitexSale> sales;
};
#else
class UnitexSales {
public:
	UnitexSales();
	~UnitexSales();
	void clear();
	bool push(UnitexSale *sale);
	bool isUsed(UnitexSale *sale);

private:
	Fifo<UnitexSale*> *pool;
	Fifo<UnitexSale*> *fifo;
};
#endif

class UnitexScanner : public MdbMasterCashlessInterface, public CodeScanner::Observer {
public:
	UnitexScanner(CodeScannerInterface *scanner, TimerEngine *timerEngine, EventEngine *eventEngine, uint32_t maxCredit);
	~UnitexScanner();

	EventDeviceId getDeviceId() override;
	void reset() override;
	bool isRefundAble() override;
	void disable() override;
	void enable() override;
	bool revalue(uint32_t credit) override;
	bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) override;
	bool saleComplete() override;
	bool saleFailed() override;
	bool closeSession() override;

	void procTimer();
	bool procCode(uint8_t *data, uint16_t dataLen) override;

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Session,
		State_Approving,
		State_Vending,
		State_Closing,
	};

	CodeScannerInterface *scanner;
	TimerEngine *timerEngine;
	Timer *timer;
	EventEngine *eventEngine;
	EventDeviceId deviceId;
	uint32_t maxCredit;
	State state;
	uint16_t productId;
	uint32_t productPrice;
	UnitexSale code;
	UnitexSales sales;

	bool stateWaitProcCode();

	void gotoStateSession();
	void stateSessionTimeout();

	void gotoStateApproving();
	bool stateApprovingProcCode();
	void stateApprovingTimeout();

	void gotoStateVending();
	void stateVendintTimeout();

	void gotoStateClosing();
	void stateClosingTimeout();
};

#endif
