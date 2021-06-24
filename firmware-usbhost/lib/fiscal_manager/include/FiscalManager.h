#ifndef LIB_FISCALMANAGER_H
#define LIB_FISCALMANAGER_H

#include "fiscal_register/include/FiscalRegister.h"
#include "cardreader/inpas/include/Inpas.h"
#include "config/include/ConfigModem.h"
#include "tcpip/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Fifo.h"
#include "utils/include/LedInterface.h"

namespace Fiscal {

class Queue {
public:
	Queue();
	~Queue();
	bool isFull();
	bool push(Fiscal::Sale *saleData);
	bool pop(Fiscal::Sale *saleData);

private:
	Fifo<Fiscal::Sale*> *pool;
	Fifo<Fiscal::Sale*> *fifo;
};

class Manager : public Fiscal::Register, public EventSubscriber {
public:
	Manager(ConfigModem *config, TimerEngine *timerEngine, EventEngineInterface *eventEngine, Fiscal::Register *fr1, QrCodeInterface *qrCodePrinter);
	virtual ~Manager();
	virtual EventDeviceId getDeviceId();
	virtual void reset();
	virtual void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	virtual void getLastSale() {}
	virtual void closeShift() {}

	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Registration,
	};

	ConfigModem *config;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	Fiscal::Register *fr1;
	QrCodeInterface *qrCodePrinter;
	State state;
	EventDeviceId deviceId;
	DecimalPointConverter converter;
	Queue fifo;
	Fiscal::Sale saleData;
	uint32_t decimalPoint;
	StringBuilder qrCodeHeader;
	StringBuilder qrCodeFooter;
	StringBuilder qrCodeData;

	void procFreeVend(Fiscal::Sale *newData);

	void stateIdleSale(Fiscal::Sale *newData);
	void gotoStateRegistration();
	void stateRegistrationSale(Fiscal::Sale *newData);
	void stateRegistrationEvent(EventEnvelope *envelope);
	void stateRegistrationEventOK();
	void stateRegistrationEventError(EventEnvelope *envelope);

	void registerSaleEvent(Fiscal::Sale *saleData);
	void registerFiscalErrorEvent(DateTime *datetime, EventEnvelope *envelope);
	void showQrCode();
};

}

#endif
