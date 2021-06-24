#ifndef COMMON_EXECUTIVE_SNIFFER_H
#define COMMON_EXECUTIVE_SNIFFER_H

#include "executive/ExeSnifferPacketLayer.h"

#include "event/include/EventEngine.h"
#include "utils/include/Event.h"
#include "uart/include/interface.h"

class ExeSnifferInterface {
public:
	virtual void reset() = 0;
};

class ExeSniffer : public ExeSnifferPacketLayer::Customer, public ExeSnifferInterface {
public:
	enum EventType {
		Event_Sale = GlobalId_Executive | 0x01, // Обнаружено событие продажи. Параметр: номер продукта (uint8_t)
	};

	enum State {
		State_NotReady = 0,
		State_Ready,
		State_Approving,
	};

	ExeSniffer(EventEngineInterface *eventEngine, StatStorage *stat);
	virtual ~ExeSniffer();
	void init(AbstractUart *slaveUart, AbstractUart *masterUart);
	virtual void reset();

	virtual void recvByte(uint8_t command, uint8_t result);
	virtual void recvData(uint8_t *data, uint16_t dataLen);
	State getState() { return state; }

private:
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	ExeSnifferPacketLayer packetLayer;
	uint8_t commandByte;
	uint8_t productId;

	void stateNotReadyCommand(uint8_t command, uint8_t result);
	void stateReadyCommand(uint8_t command, uint8_t result);
	void stateApprovingCommand(uint8_t command, uint8_t result);
};

#endif
