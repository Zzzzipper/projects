#ifndef DEX_SERVER_H_
#define DEX_SERVER_H_

#include "dex/DexProtocol.h"
#include "dex/DexReceiver.h"
#include "utils/include/Event.h"

#include <stdint.h>

class AbstractUart;
class TimerEngine;
class Timer;

namespace Dex {
class DataGenerator;
class DataParser;
class CommandResult;
class Sender;

class Server : public Receiver::Customer {
public:
	enum EventType {
		Event_ManufacturerMode = GlobalId_Dex | 0x01,
	};

	enum State {
		State_Idle = 0,
		State_Wait,

		State_Master_Restart,
		State_Master_FHS_1,
		State_Master_FHS_2,
		State_Master_SHS_1,
		State_Master_SHS_2,
		State_Master_SHS_3,

		State_Slave_FHS_1,
		State_Slave_FHS_2,
		State_Slave_SHS_1,
		State_Slave_SHS_2,
		State_Slave_SHS_3,

		State_SendData_1,
		State_SendData_2,
		State_SendData_3,
		State_SendData_4,

		State_RecvData_1,
		State_RecvData_2,
		State_RecvData_3,
	};

	Server(EventObserver *observer = NULL);
	virtual ~Server();
	void init(AbstractUart *uart, TimerEngine *timers, DataGenerator *dataGenerator, DataParser *dataParser);
	void reset();
	void sendData(DataGenerator *dataGenerator, CommandResult *commandResult);
	void recvData(DataParser *dataParser, CommandResult *commandResult);
	void manufacturer(CommandResult *commandResult);
	State getState() { return state; }

private:
	EventCourier courier;
	AbstractUart *uart;
	Timer *timer;
	Sender *sender;
	Receiver *receiver;
	State state;
	DexHandShakeRequest reqPkt;
	DexHandShakeResponse respPkt;
	DataGenerator *slaveDataGenerator;
	DataParser *slaveDataParser;
	uint8_t masterCommand;
	uint8_t tryNumber;
	CommandResult *masterCommandResult;
	DataGenerator *masterDataGenerator;
	DataParser *masterDataParser;
	uint8_t command;
	CommandResult *commandResult;
	DataGenerator *dataGenerator;
	DataParser *dataParser;

	void procSuccess();
	void procError();

	void startMasterFHS();
	void stateMasterFHS1Control(DexControl control);
	void stateMasterFHS1Confirm(const uint8_t number);
	void stateMasterFHS2Confirm(const uint8_t number);
	void stateMasterSHS1Control(DexControl control);
	void stateMasterSHS2Control(DexControl control);
	void stateMasterSHS2Data(const uint8_t *data, const uint16_t len, bool last);
	void stateMasterSHS3Control(DexControl control);

	void stateWaitControl(DexControl control);
	void stateSlaveFHS1Confirm(const uint8_t number);
	void stateSlaveFHS1Control(DexControl control);
	void stateSlaveFHS1Data(const uint8_t *data, const uint16_t len, bool last);
	void stateSlaveFHS2Control(DexControl control);
	void stateSlaveSHS1Timeout();
	void stateSlaveSHS2Confirm(const uint8_t number);
	void stateSlaveSHS3Confirm(const uint8_t number);

	void startSend(DataGenerator *dataGenerator);
	void stateSendData1Timeout();
	void stateSendData2Confirm(const uint8_t number);
	void stateSendData3Control(DexControl control);
	void stateSendData3Confirm(const uint8_t number);
	void stateSendData4Confirm(const uint8_t number);

	void startRecv(DataParser *dataParser);
	void stateRecvData1Control(DexControl control);
	void stateRecvData2Control(DexControl control);
	void stateRecvData2Data(const uint8_t *data, const uint16_t len, bool last);
	void stateRecvData3Control(DexControl control);
	void stateRecvDataCancel();

public:
	void procControl(DexControl control) override;
	void procData(const uint8_t *data, const uint16_t len, bool last) override;
	void procConfirm(const uint8_t number) override;
	void procWrongCrc() override;
	void procTimer();
};
}

#endif
