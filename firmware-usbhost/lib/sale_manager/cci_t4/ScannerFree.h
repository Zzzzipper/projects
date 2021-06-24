#if 0
#ifndef LIB_SALEMANAGER_CCI_SCANNERFREE_H_
#define LIB_SALEMANAGER_CCI_SCANNERFREE_H_

#include "mdb/master/cashless/MdbMasterCashless.h"
#include "code_scanner/CodeScanner.h"
#include "common/utils/include/Fifo.h"

class ScannerFree : public MdbMasterCashlessInterface, public CodeScanner::Observer {
public:
	ScannerFree(CodeScannerInterface *scanner, EventEngine *eventEngine);

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

	bool procCode(uint8_t *data, uint16_t dataLen) override;

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Session,
		State_Approving,
		State_Vending,
	};

	CodeScannerInterface *scanner;
	EventEngine *eventEngine;
	EventDeviceId deviceId;
	State state;
	uint32_t productPrice;


};

#endif
#else
#ifndef LIB_SALEMANAGER_CCI_SCANNERFREE_H_
#define LIB_SALEMANAGER_CCI_SCANNERFREE_H_

#include "mdb/master/cashless/MdbMasterCashless.h"
#include "code_scanner/CodeScanner.h"
#include "common/utils/include/Fifo.h"

class ScannerFree : public MdbMasterCashlessInterface, public CodeScanner::Observer {
public:
	ScannerFree(CodeScannerInterface *scanner, TimerEngine *timerEngine, EventEngine *eventEngine, uint32_t maxCredit);

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
	uint32_t productPrice;

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
#endif
