#if 1
#ifndef LIB_SALEMANAGER_EXEMASTER_EXEMASTER_H_
#define LIB_SALEMANAGER_EXEMASTER_EXEMASTER_H_

#include "ExeMasterPacketLayer.h"

#include "common/event/include/EventEngine.h"
#include "common/config/include/StatStorage.h"

class ExeMasterInterface {
public:
	enum EventType {
		Event_NotReady		 = GlobalId_Executive | 0x01,
		Event_Ready			 = GlobalId_Executive | 0x02,
		Event_VendRequest	 = GlobalId_Executive | 0x03, // uint8_t ProductId
		Event_VendPrice		 = GlobalId_Executive | 0x04,
		Event_VendComplete   = GlobalId_Executive | 0x05,
		Event_VendFailed     = GlobalId_Executive | 0x06,
	};
	virtual void reset() = 0;
	virtual void setDicimalPoint(uint8_t decimalPoint) = 0;
	virtual void setScalingFactor(uint8_t scalingFactor) = 0;
	virtual void setChange(bool change) = 0;
	virtual void setCredit(uint32_t credit) = 0;
	virtual void setPrice(uint32_t credit) = 0;
	virtual void approveVend(uint32_t credit) = 0;
	virtual void denyVend(uint32_t price) = 0;
	virtual bool isEnabled() = 0;
};

class ExeMaster : public ExeMasterInterface, public ExeMasterPacketLayer::Customer {
public:
	ExeMaster(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine, StatStorage *stat);
	virtual ~ExeMaster();
	virtual void reset();
	virtual void setDicimalPoint(uint8_t decimalPoint) override;
	virtual void setScalingFactor(uint8_t scalingFactor) override;
	virtual void setChange(bool change) override;
	virtual void setCredit(uint32_t credit) override;
	virtual void setPrice(uint32_t credit) override;
	virtual void approveVend(uint32_t credit) override;
	virtual void denyVend(uint32_t price) override;
	virtual bool isEnabled() override;

	virtual void recvByte(uint8_t byte) override;
	virtual void recvTimeout() override;

private:
	enum State {
		State_Idle = 0,
		State_NotFound,
		State_NotReady,
		State_ReadyStatus,
		State_ReadyCredit,
		State_CreditShow,
		State_CreditDelay,
		State_PriceShow,
		State_Approve,
		State_Vending,
		State_PriceDelay,
	};
	enum Command {
		Command_None = 0,
		Command_ChangeCredit,
		Command_VendPrice,
		Command_VendApprove,
		Command_VendDeny,
	};

	EventEngineInterface *eventEngine;
	ExeMasterPacketLayer *packetLayer;
	TimerEngine *timers;
	Timer *timer;
	EventDeviceId deviceId;
	StatNode *state;
	Command command;
	uint8_t decimalPoint;
	uint8_t scalingFactor;
	bool change;
	uint32_t credit;
	uint32_t price;
	uint32_t repeatCount;

	void procTimer();

	void gotoStateNotFound();
	void stateNotFoundSend();
	void stateNotFoundRecv(uint8_t b);

	void gotoStateNotReady();
	void stateNotReadySend();
	void stateNotReadyRecv(uint8_t b);

	void gotoStateReadyStatus();
	void stateReadyStatusSend();
	void stateReadyStatusRecv(uint8_t b);
	void gotoStateReadyCredit();
	void stateReadyCreditSend();
	void stateReadyCreditRecv(uint8_t b);
	void stateReadyTimeout();
	bool checkReadyCommand();

	void gotoStateCreditShow();
	void stateCreditShowSend();
	void stateCreditShowRecv();
	void gotoStateCreditDelay();
	void stateCreditDelaySend();
	void stateCreditDelayRecv(uint8_t b);

	void gotoStatePriceShow();
	void statePriceShowSend();
	void statePriceShowRecv();
	void statePriceShowTimeout();
	void gotoStatePriceDelay();
	void statePriceDelaySend();
	void statePriceDelayRecv(uint8_t b);

	void gotoStateApprove();
	void stateApproveTimeout();

	void gotoStateVending();
	void stateVendingSend();
	void stateVendingRecv(uint8_t byte);
	void stateVendingRecvTimeout();
};
#endif
#else
#ifndef LIB_SALEMANAGER_EXEMASTER_EXEMASTER_H_
#define LIB_SALEMANAGER_EXEMASTER_EXEMASTER_H_

#include "ExeMasterPacketLayer.h"

#include "common/event/include/EventEngine.h"

class ExeMasterInterface {
public:
	enum EventType {
		Event_NotReady		 = GlobalId_Executive | 0x01,
		Event_Ready			 = GlobalId_Executive | 0x02,
		Event_VendRequest	 = GlobalId_Executive | 0x03, // uint8_t ProductId
		Event_VendPrice		 = GlobalId_Executive | 0x04,
		Event_VendComplete   = GlobalId_Executive | 0x05,
		Event_VendFailed     = GlobalId_Executive | 0x06,
	};
	virtual void reset() = 0;
	virtual void setDicimalPoint(uint8_t decimalPoint) = 0;
	virtual void setScalingFactor(uint8_t scalingFactor) = 0;
	virtual void setChange(bool change) = 0;
	virtual void setCredit(uint32_t credit) = 0;
	virtual void setPrice(uint32_t credit) = 0;
	virtual void approveVend(uint32_t credit) = 0;
	virtual void denyVend(uint32_t price) = 0;
	virtual bool isEnabled() = 0;
};

class ExeMaster : public ExeMasterInterface, public ExeMasterPacketLayer::Customer {
public:
	ExeMaster(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~ExeMaster();
	virtual void reset();
	virtual void setDicimalPoint(uint8_t decimalPoint) override;
	virtual void setScalingFactor(uint8_t scalingFactor) override;
	virtual void setChange(bool change) override;
	virtual void setCredit(uint32_t credit) override;
	virtual void setPrice(uint32_t credit) override;
	virtual void approveVend(uint32_t credit) override;
	virtual void denyVend(uint32_t price) override;
	virtual bool isEnabled() override;

	virtual void send();
	virtual void recvByte(uint8_t byte);
	virtual void recvTimeout();

	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_NotFound,
		State_NotReady,
		State_ReadyStatus,
		State_ReadyCredit,
		State_CreditShow,
		State_CreditDelay,
		State_PriceShow,
		State_PriceDelay,
		State_VendCredit,
		State_Vending,
	};
	enum Command {
		Command_None = 0,
		Command_ChangeCredit,
		Command_VendPrice,
		Command_VendApprove,
		Command_VendDeny,
	};

	EventEngineInterface *eventEngine;
	ExeMasterPacketLayer *packetLayer;
	TimerEngine *timers;
	Timer *timer;
	EventDeviceId deviceId;
	State state;
	Command command;
	uint8_t decimalPoint;
	uint8_t scalingFactor;
	bool change;
	uint32_t credit;
	uint32_t price;

	void stateNotReadySend();
	void stateNotReadyRecv(uint8_t b);

	void stateReadyStatusSend();
	void stateReadyStatusRecv(uint8_t b);
	void stateReadyCreditSend();
	void stateReadyCreditRecv(uint8_t b);
	void stateReadyTimeout();
	bool checkReadyCommand();

	void gotoStateCreditShow();
	void stateCreditShowSend();
	void stateCreditShowRecv();
	void stateCreditDelaySend();
	void stateCreditDelayRecv(uint8_t b);
	void stateCreditDelayTimeout();

	void gotoStatePriceShow();
	void statePriceShowSend();
	void statePriceShowRecv();
	void statePriceDelaySend();
	void statePriceDelayRecv(uint8_t b);
	void statePriceDelayTimeout();

/*	void stateReadyPriceSend();
	void stateReadyPriceRecv();

	void gotoStateCreditChange();
	void stateCreditChangeCreditSend();
	void stateCreditChangeCreditRecv();
	void stateCreditStatusSend();
	void stateCreditStatusRecv();
	void stateCreditCreditSend();
	void stateCreditCreditRecv(uint8_t b);
	void stateCreditPriceSend();
	void stateCreditPriceRecv();
	void stateCreditTimeout();
	bool checkCreditCommand();
*/
	void gotoStateVendCredit();
	void stateVendCreditSend();
	void stateVendCreditRecv();
	void stateVendingSend();
	void stateVendingRecv(uint8_t byte);
	void stateVendingRecvTimeout();
};
#endif
#endif
