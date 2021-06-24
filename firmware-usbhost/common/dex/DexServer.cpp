#include "DexSender.h"
#include "DexReceiver.h"
#include "include/DexServer.h"
#include "include/DexDataGenerator.h"
#include "include/DexDataParser.h"

#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"
#include "common.h"

namespace Dex {

Server::Server(EventObserver *observer) : courier(observer), uart(NULL), timer(NULL), state(State_Idle) {
	this->sender = new Sender();
	this->receiver = new Receiver(this);
}

Server::~Server() {
	delete this->receiver;
	delete this->sender;
}

/*
 * uart - указатель на UART, не может быть равен NULL!
 */
void Server::init(AbstractUart *uart, TimerEngine *timers, DataGenerator *dataGenerator, DataParser *dataParser) {
	LOG_INFO(LOG_DEX, "Dex init");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_DEX, "Wrong state " << this->state);
		return;
	}
	this->uart = uart;
	this->sender->init(uart, timers);
	this->timer = timers->addTimer<Server, &Server::procTimer>(this);
	this->slaveDataGenerator = dataGenerator;
	this->slaveDataParser = dataParser;
	reset();
}

void Server::reset() {
	LOG_ERROR(LOG_DEX, "Dex reset");
	this->command = DexCommand_Unsupported;
	this->commandResult = NULL;
	this->sender->resetParity();
	this->receiver->setConnection(uart);
	this->receiver->reset();
	this->timer->stop();
	this->state = State_Wait;
}

void Server::sendData(DataGenerator *dataGenerator, CommandResult *commandResult) {
	if(this->state == State_Idle) {
		if(commandResult != NULL) { commandResult->error(); }
		return;
	}
	this->masterCommand = DexCommand_MasterSend;
	this->masterDataGenerator = dataGenerator;
	this->masterCommandResult = commandResult;
	this->tryNumber = DEX_TRY_MAX;
	if(this->state == State_Wait) {
		startMasterFHS();
	}
}

void Server::recvData(DataParser *dataParser, CommandResult *commandResult) {
	if(this->state == State_Idle) {
		if(commandResult != NULL) { commandResult->error(); }
		return;
	}
	this->masterCommand = DexCommand_MasterRecv;
	this->masterDataParser = dataParser;
	this->masterCommandResult = commandResult;
	this->tryNumber = DEX_TRY_MAX;
	if(this->state == State_Wait) {
		startMasterFHS();
	}
}

void Server::manufacturer(CommandResult *commandResult) {
	if(this->state == State_Idle) {
		if(commandResult != NULL) { commandResult->error(); }
		return;
	}
	this->masterCommand = DexCommand_MasterManufacturer;
	this->masterCommandResult = commandResult;
	this->tryNumber = DEX_TRY_MAX;
	if(this->state == State_Wait) {
		startMasterFHS();
	}
}

void Server::procSuccess() {
	CommandResult *p = commandResult;
	reset();
	if(p != NULL) {
		p->success();
	}
}

void Server::procError() {
	LOG_ERROR(LOG_DEX, "procError " << state << "," << command << "," << tryNumber);
	if(command == DexCommand_MasterRecv || command == DexCommand_MasterSend || command == DexCommand_MasterConfig) {
		if(tryNumber > 0) {
			LOG_ERROR(LOG_DEX, "Next try. " << tryNumber << " retries left.");
			tryNumber--;
			sender->resetParity();
			receiver->reset();
			timer->start(DEX_TIMEOUT_RESTART);
			state = State_Master_Restart;
			return;
		}
	}
	LOG_ERROR(LOG_DEX, "Too many tries.");
	CommandResult *p = commandResult;
	reset();
	if(p != NULL) {
		p->error();
	}
}

void Server::procTimer() {
	LOG_INFO(LOG_DEX, "procTimer " << state);
	switch(this->state) {
		case State_Idle:
		case State_Wait:
			break;
		case State_Master_Restart:
			startMasterFHS(); break;
		case State_Master_FHS_1:
		case State_Master_FHS_2:
		case State_Master_SHS_1:
		case State_Master_SHS_2:
		case State_Master_SHS_3:
		case State_Slave_FHS_1:
		case State_Slave_FHS_2:
			procError(); break;
		case State_Slave_SHS_1:
			stateSlaveSHS1Timeout(); break;
		case State_Slave_SHS_2:
		case State_Slave_SHS_3:
			procError(); break;
		case State_SendData_1:
			stateSendData1Timeout(); break;
		case State_SendData_2:
		case State_SendData_3:
		case State_SendData_4:
		case State_RecvData_1:
			procError(); break;
		case State_RecvData_2:
		case State_RecvData_3:
			stateRecvDataCancel(); break;
	}
}

void Server::procControl(DexControl control) {
	LOG_DEBUG(LOG_DEX, "procControl " << state);
	switch(state) {
		case State_Wait: stateWaitControl(control); return;
		case State_Master_Restart: return;
		case State_Master_FHS_1: stateMasterFHS1Control(control); return;
		case State_Master_SHS_1: stateMasterSHS1Control(control); return;
		case State_Master_SHS_2: stateMasterSHS2Control(control); return;
		case State_Master_SHS_3: stateMasterSHS3Control(control); return;
		case State_Slave_FHS_1: stateSlaveFHS1Control(control); return;
		case State_Slave_FHS_2: stateSlaveFHS2Control(control); return;
		case State_SendData_3: stateSendData3Control(control); return;
		case State_RecvData_1: stateRecvData1Control(control); return;
		case State_RecvData_2: stateRecvData2Control(control); return;
		case State_RecvData_3: stateRecvData3Control(control); return;
		default: LOG_ERROR(LOG_DEX, "Unwaited control " << state << "," << control);
	}
}

void Server::procData(const uint8_t *data, const uint16_t len, bool last) {
	LOG_DEBUG(LOG_DEX, "procData " << state);
	switch(state) {
		case State_Master_SHS_2: stateMasterSHS2Data(data, len, last); return;
		case State_Slave_FHS_1: stateSlaveFHS1Data(data, len, last); return;
		case State_RecvData_2: stateRecvData2Data(data, len, last); return;
		default: LOG_ERROR(LOG_DEX, "Unwaited data " << len);
	}
}

void Server::procConfirm(const uint8_t number) {
	LOG_DEBUG(LOG_DEX, "procConfirm " << state);
	switch(state) {
		case State_Master_FHS_1: stateMasterFHS1Confirm(number); return;
		case State_Master_FHS_2: stateMasterFHS2Confirm(number); return;
		case State_Slave_FHS_1: stateSlaveFHS1Confirm(number); return;
		case State_Slave_SHS_2: stateSlaveSHS2Confirm(number); return;
		case State_Slave_SHS_3: stateSlaveSHS3Confirm(number); return;
		case State_SendData_2: stateSendData2Confirm(number); return;
		case State_SendData_3: stateSendData3Confirm(number); return;
		case State_SendData_4: stateSendData4Confirm(number); return;
		default: LOG_ERROR(LOG_DEX, "Unwaited confirm " << number);
	}
}

void Server::procWrongCrc() {
	LOG_DEBUG(LOG_DEX, "procWrongCrc " << state);
	sender->sendControl(DexControl_NAK);
}

void Server::startMasterFHS() {
	LOG_INFO(LOG_DEX, "Start master FHS");
	command = masterCommand;
	commandResult = masterCommandResult;

	sender->sendControl(DexControl_ENQ);

	timer->start(DEX_TIMEOUT_ENQ_WAIT);
	state = State_Master_FHS_1;
}

void Server::stateMasterFHS1Control(DexControl control) {
	if(control != DexControl_ENQ) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited control.  [" << control << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv ENQ >>>>>>>>>>>>>>>>>>>>");
}

void Server::stateMasterFHS1Confirm(const uint8_t number) {
	if(number != '0') {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited confirm. [" << number << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv DLE 0");
	switch(command) {
		case DexCommand_MasterSend: reqPkt.set(DexOperation_Send); break;
		case DexCommand_MasterRecv: reqPkt.set(DexOperation_Recv); break;
		case DexCommand_MasterManufacturer: reqPkt.set(DexOperation_Manufacturer); break;
		default: {
			LOG_ERROR(LOG_DEX, "Unsupported operation " << command);
			procError();
			return;
		}
	}
	sender->sendData(&reqPkt, sizeof(reqPkt), DexControl_SOH, DexControl_ETX);

	timer->start(DEX_TIMEOUT_READING);
	state = State_Master_FHS_2;
}

void Server::stateMasterFHS2Confirm(const uint8_t number) {
	if(number != '1') {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited confirm.  [" << number << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv DLE 1");
	sender->sendControl(DexControl_EOT);

	timer->start(DEX_TIMEOUT_READING);
	state = State_Master_SHS_1;
}

void Server::stateMasterSHS1Control(DexControl control) {
	if(control != DexControl_ENQ) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited control.  [" << control << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv ENQ");
	sender->sendConfirm();

	timer->start(DEX_TIMEOUT_READING);
	state = State_Master_SHS_2;
}

void Server::stateMasterSHS2Control(DexControl control) {
	if(control != DexControl_ENQ) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited control.  [" << control << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv ENQ");
	sender->repeatConfirm();
	timer->start(DEX_TIMEOUT_READING);
}

void Server::stateMasterSHS2Data(const uint8_t *data, const uint16_t len, bool last) {
	if(len < sizeof(DexHandShakeResponse) || last == false) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Wrong data size. " << len);
		procError();
		return;
	}

	DexHandShakeResponse *pkt = (DexHandShakeResponse*)data;
	LOG_INFO(LOG_DEX, "Recv response code " << pkt->responseCode[0] << pkt->responseCode[1]);
	if(pkt->isResponseCode(DexResponse_OK) == false) {
		LOG_ERROR(LOG_DEX, "Unwaited response code " << pkt->responseCode[0] << pkt->responseCode[1]);
		command = DexCommand_Unsupported;
	}
	sender->sendConfirm();

	timer->start(DEX_TIMEOUT_READING);
	state = State_Master_SHS_3;
}

void Server::stateMasterSHS3Control(DexControl control) {
	if(control != DexControl_EOT) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited request. [" << control << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv EOT");
	switch(command) {
		case DexCommand_MasterRecv: startRecv(masterDataParser); break;
		case DexCommand_MasterSend: startSend(masterDataGenerator); break;
		case DexCommand_MasterConfig: startSend(masterDataGenerator); break;
		case DexCommand_MasterManufacturer: {
			LOG_INFO(LOG_DEX, "Manufacturer mode started");
			timer->stop();
			if(commandResult != NULL) { commandResult->success(); }
			return;
		}
		default: {
			LOG_ERROR(LOG_DEX, "Unsupported operation");
			procError();
			return;
		}
	}
}

void Server::stateWaitControl(DexControl control) {
	if(control != DexControl_ENQ) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited control. [" << control << "] ");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv ENQ");

	sender->sendConfirm();
	timer->start(DEX_TIMEOUT_READING);
	state = State_Slave_FHS_1;
}

void Server::stateSlaveFHS1Confirm(const uint8_t number) {
	LOG_INFO(LOG_DEX, "stateSlaveFHS1Confirm " << state << "," << number);
	command = DexCommand_MasterSend;
	dataGenerator = slaveDataGenerator;
	commandResult = NULL;
	tryNumber = 1;
	sender->resetParity();
	stateMasterFHS1Confirm(number);
}

void Server::stateSlaveFHS1Control(DexControl control) {
	if(control != DexControl_ENQ) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited control. [" << control << "] ");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv ENQ");
	sender->sendConfirm();

	timer->start(DEX_TIMEOUT_READING);
	state = State_Slave_FHS_1;
}

void Server::stateSlaveFHS1Data(const uint8_t *data, const uint16_t len, bool last) {
	if(len != sizeof(DexHandShakeRequest) || last == false) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Wrong data size. [" << len << "," << last << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv Master Info");
	DexHandShakeRequest *pkt = (DexHandShakeRequest*)data;
	switch(pkt->operation) {
		case DexOperation_Recv: {
			if(slaveDataGenerator == NULL) {
				LOG_ERROR(LOG_DEX, "DataGenerator not defined");
				command = DexCommand_Unsupported;
				break;
			}
			command = DexCommand_SlaveRecv;
			break;
		}
		case DexOperation_Send: {
			if(slaveDataParser == NULL) {
				LOG_ERROR(LOG_DEX, "DataParser not defined");
				command = DexCommand_Unsupported;
				break;
			}
			command = DexCommand_SlaveSend;
			break;
		}
		case DexOperation_Manufacturer: {
			command = DexCommand_SlaveManufacturer;
			break;
		}
		default: {
			command = DexCommand_Unsupported;
			break;
		}
	}
	LOG_INFO_STR(LOG_DEX, pkt->communicationId, sizeof(pkt->communicationId));
	LOG_INFO(LOG_DEX, "operation " << pkt->operation);
	LOG_INFO_STR(LOG_DEX, pkt->revision, sizeof(pkt->revision));
	LOG_INFO_STR(LOG_DEX, pkt->level, sizeof(pkt->level));

	sender->sendConfirm();
	timer->start(DEX_TIMEOUT_READING);
	state = State_Slave_FHS_2;
}

void Server::stateSlaveFHS2Control(DexControl control) {
	if(control != DexControl_EOT) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited request.  [" << control << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv EOT");

	timer->start(DEX_TIMEOUT_INTERSESSION);
	state = State_Slave_SHS_1;
}

void Server::stateSlaveSHS1Timeout() {
	LOG_INFO(LOG_DEX, "SHS1 timeout");

	sender->sendControl(DexControl_ENQ);
	timer->start(DEX_TIMEOUT_READING);
	state = State_Slave_SHS_2;
}

void Server::stateSlaveSHS2Confirm(const uint8_t number) {
	if(number != '0') {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited confirm.  [" << number << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv DLE 0");

	if(command == DexCommand_Unsupported) {
		respPkt.set(DexResponse_UndefinedError);
	} else {
		respPkt.set(DexResponse_OK);
	}
	sender->sendData(&respPkt, sizeof(respPkt), DexControl_SOH, DexControl_ETX);
	timer->start(DEX_TIMEOUT_READING);
	state = State_Slave_SHS_3;
}

void Server::stateSlaveSHS3Confirm(const uint8_t number) {
	if(number != '1') {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited confirm.  [" << number << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv DLE 1");

	sender->sendControl(DexControl_EOT);
	switch(command) {
		case DexCommand_SlaveRecv: startSend(slaveDataGenerator); break;
		case DexCommand_SlaveSend: startRecv(slaveDataParser); break;
		case DexCommand_SlaveManufacturer: {
			LOG_INFO(LOG_DEX, "Manufacturer mode started");
			timer->stop();
			courier.deliver(Event_ManufacturerMode);
			return;
		}
		default: {
			LOG_ERROR(LOG_DEX, "Unsupported operation");
			procError();
		}
	}
}

void Server::startSend(DataGenerator *dataGenerator) {
	LOG_INFO(LOG_DEX, "Start send");
	this->dataGenerator = dataGenerator;
	this->timer->start(DEX_TIMEOUT_INTERSESSION);
	this->state = State_SendData_1;
}

void Server::stateSendData1Timeout() {
	LOG_INFO(LOG_DEX, "SendData1 timeout");
	sender->sendControl(DexControl_ENQ);

	timer->start(DEX_TIMEOUT_READING);
	state = State_SendData_2;
}

// todo: разобраться при каком условии делать перепосылку данных
void Server::stateSendData2Confirm(const uint8_t number) {
	if(number != '0') {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited confirm.  [" << number << "]");
		procError();
		return;
	}

	dataGenerator->reset();
	if(dataGenerator->isLast() == false) {
		LOG_INFO(LOG_DEX, "send pkt " << dataGenerator->getLen());
		LOG_DEBUG_STR(LOG_DEX, (char*)dataGenerator->getData(), dataGenerator->getLen());
		sender->sendData(dataGenerator->getData(), dataGenerator->getLen(), DexControl_STX, DexControl_ETB);
		timer->start(DEX_TIMEOUT_READING);
		state = State_SendData_3;
	} else {
		LOG_INFO(LOG_DEX, "send last pkt " << dataGenerator->getLen());
		LOG_DEBUG_STR(LOG_DEX, (char*)dataGenerator->getData(), dataGenerator->getLen());
		sender->sendData(dataGenerator->getData(), dataGenerator->getLen(), DexControl_STX, DexControl_ETX);
		timer->start(DEX_TIMEOUT_READING);
		state = State_SendData_4;
	}
}

void Server::stateSendData3Control(DexControl control) {
	if(control == DexControl_NAK) {
		LOG_INFO(LOG_DEX, "repeat pkt " << dataGenerator->getLen());
		LOG_DEBUG_STR(LOG_DEX, (char*)dataGenerator->getData(), dataGenerator->getLen());
		sender->sendData(dataGenerator->getData(), dataGenerator->getLen(), DexControl_STX, DexControl_ETB);
		timer->start(DEX_TIMEOUT_READING);
	}
}

void Server::stateSendData3Confirm(const uint8_t number) {
	if(number != '0' && number != '1') {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited confirm.  [" << number << "]");
		procError();
		return;
	}

	dataGenerator->next();
	if(dataGenerator->isLast() == false) {
		LOG_INFO(LOG_DEX, "send pkt " << dataGenerator->getLen());
		LOG_DEBUG_STR(LOG_DEX, (char*)dataGenerator->getData(), dataGenerator->getLen());
		sender->sendData(dataGenerator->getData(), dataGenerator->getLen(), DexControl_STX, DexControl_ETB);
		timer->start(DEX_TIMEOUT_READING);
		state = State_SendData_3;
	} else {
		LOG_INFO(LOG_DEX, "send last pkt " << dataGenerator->getLen());
		LOG_DEBUG_STR(LOG_DEX, (char*)dataGenerator->getData(), dataGenerator->getLen());
		sender->sendData(dataGenerator->getData(), dataGenerator->getLen(), DexControl_STX, DexControl_ETX);
		timer->start(DEX_TIMEOUT_READING);
		state = State_SendData_4;
	}
}

void Server::stateSendData4Confirm(const uint8_t number) {
	if(number != '0' && number != '1') {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited confirm.  [" << number << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Send complete");
	sender->sendControl(DexControl_EOT);
	procSuccess();
}

void Server::startRecv(DataParser *dataParser) {
	LOG_INFO(LOG_DEX, "Start receive");
	this->dataParser = dataParser;
	this->sender->resetParity();
	this->timer->start(DEX_TIMEOUT_READING);
	this->state = State_RecvData_1;
}

void Server::stateRecvData1Control(DexControl control) {
	if(control != DexControl_ENQ) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited control.  [" << control << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv ENQ");
	dataParser->start(0);
	sender->sendConfirm();
	timer->start(DEX_TIMEOUT_READING);
	state = State_RecvData_2;
}

void Server::stateRecvData2Control(DexControl control) {
	if(control != DexControl_ENQ) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited control.  [" << control << "]");
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv ENQ again");
}

void Server::stateRecvData2Data(const uint8_t *data, const uint16_t len, bool last) {
	dataParser->procData(data, len);
	sender->sendConfirm();

	if(last == false) {
		timer->start(DEX_TIMEOUT_READING);
	} else {
		timer->start(DEX_TIMEOUT_READING);
		state = State_RecvData_3;
	}
}

void Server::stateRecvData3Control(DexControl control) {
	if(control != DexControl_EOT) {
		LOG_ERROR(LOG_DEX, "DexServer[" << this->state << "] Unwaited request. [" << control << "]");
		dataParser->error();
		procError();
		return;
	}

	LOG_INFO(LOG_DEX, "Recv EOT");
	dataParser->complete();
	procSuccess();
}

void Server::stateRecvDataCancel() {
	dataParser->error();
	procError();
}
}
