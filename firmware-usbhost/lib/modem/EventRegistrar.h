#ifndef LIB_MODEM_EVENTREGISTRAR_H_
#define LIB_MODEM_EVENTREGISTRAR_H_

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"

#define AUTOMAT_DISABLE_TIMEOUT 60*60*1000

class EventRegistrar : public EventSubscriber {
public:
	enum EventType {
		Event_CriticalError = GlobalId_EventRegistrar | 0x01,
	};

	EventRegistrar(ConfigModem *config, TimerEngine *timers, EventEngineInterface *eventEngine, RealTimeInterface *realtime);
	~EventRegistrar();
	void setObserver(EventObserver *observer);
	void reset();
	virtual void proc(EventEnvelope *envelope);
	void procTimer();

	void registerCashlessIdNotFoundEvent(uint16_t cashlessId, Mdb::DeviceContext *context);
	void registerPriceListNotFoundEvent(const char *deviceId, uint8_t priceListNumber, Mdb::DeviceContext *context);
	void registerPriceNotEqualEvent(const char *selectId, uint32_t expectedPrice, uint32_t actualPrice, Mdb::DeviceContext *context);
	void registerCoinInEvent(uint32_t value, uint8_t route);
	void registerBillInEvent(uint32_t value, uint8_t route);
	void registerChangeEvent(uint32_t value);
	void registerDeviceError(EventEnvelope *envelope);
	void registerCashCanceledEvent();
	void registerVendFailedEvent(const char *selectId, Mdb::DeviceContext *context);
	void startSession();
	void registerSessionClosedByMaster();
	void registerSessionClosedByTimeout();
	void registerSessionClosedByTerminal();
	void registerCashlessCanceledEvent(const char *selectId);
	void registerCashlessDeniedEvent(const char *selectId);

private:
	enum State {
		State_Idle = 0,
		State_SaleDisabled,
		State_SaleDisabledTooLong,
		State_SaleEnabled
	};

	ConfigModem *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	RealTimeInterface *realtime;
	Timer *timer;
	State state;
	EventCourier courier;
	uint16_t vendFailed;
	uint32_t sessionStart;

	void gotoStateSaleDisabled();
	void stateSaleDisabledEvent(EventEnvelope *envelope);
	void stateSaleDisabledEventAutomatState(EventEnvelope *envelope);
	void stateSaleDisabledTimeout();

	void gotoStateSaleDisabledTooLong();
	void stateSaleDisabledTooLongEvent(EventEnvelope *envelope);
	void stateSaleDisabledTooLongEventAutomatState(EventEnvelope *envelope);

	void gotoStateSaleEnabled();
	void stateSaleEnabledEvent(EventEnvelope *envelope);
	void stateSaleEnabledEventAutomatState(EventEnvelope *envelope);
};

#endif
