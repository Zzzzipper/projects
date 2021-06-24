#ifndef LIB_SALEMANAGER_H_
#define LIB_SALEMANAGER_H_

#include "common/utils/include/Event.h"
#include "common/event/include/EventEngine.h"

class SaleManager {
public:
	enum EventType {
		Event_AutomatState	= GlobalId_SaleManager | 0x01, // uint8_t (false - отключен, true - включен)
		Event_Shutdown		= GlobalId_SaleManager | 0x02,
		Event_PowerDown		= GlobalId_SaleManager | 0x03,
	};

	virtual ~SaleManager() {}
	virtual void reset() = 0;
	virtual void service() {}
	virtual void shutdown() {}
};

class DummySaleManager : public SaleManager {
public:
	DummySaleManager(EventEngine *eventEngine) : eventEngine(eventEngine), deviceId(eventEngine) {}
	void reset() override {};
	void shutdown() override {
		EventInterface event(deviceId, SaleManager::Event_Shutdown);
		eventEngine->transmit(&event);
	}

private:
	EventEngine *eventEngine;
	EventDeviceId deviceId;
};

#endif
