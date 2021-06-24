#include "sim900/include/GsmTcpConnection.h"
#include "sim900/include/GsmDriver.h"

#include "utils/include/Event.h"
#include "utils/include/StringParser.h"
#include "timer/include/TimerEngine.h"
#include "mdb/MdbProtocol.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#include <string.h>

#define FAKE_CLOSE_TIMEOUT 1
#define PACKET_SIZE 768

namespace Gsm {

TcpConnection::TcpConnection(TimerEngine *timers, CommandProcessor *commandProcessor, uint16_t id, StatStorage *stat) :
	timers(timers),
	commandProcessor(commandProcessor),
	eventPrefix(3, 3)
{
	switch(id) {
	case 0: state = stat->add(Mdb::DeviceContext::Info_Gsm_TcpConn0, State_Idle); break;
	case 1: state = stat->add(Mdb::DeviceContext::Info_Gsm_TcpConn1, State_Idle); break;
	case 2: state = stat->add(Mdb::DeviceContext::Info_Gsm_TcpConn2, State_Idle); break;
	case 3: state = stat->add(Mdb::DeviceContext::Info_Gsm_TcpConn3, State_Idle); break;
	case 4: state = stat->add(Mdb::DeviceContext::Info_Gsm_TcpConn4, State_Idle); break;
	case 5: state = stat->add(Mdb::DeviceContext::Info_Gsm_TcpConn5, State_Idle); break;
	}
	wrongStateCount = stat->add(Mdb::DeviceContext::Info_Tcp_WrongStateCount, 0);
	executeErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_ExecuteErrorCount, 0);
	cipStatusErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_CipStatusErrorCount, 0);
	gprsErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_GprsErrorCount, 0);
	cipSslErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_CipSslErrorCount, 0);
	cipStartErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_CipStartErrorCount, 0);
	connectErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_ConnectErrorCount, 0);
	sendErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_SendErrorCount, 0);
	recvErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_RecvErrorCount, 0);
	unwaitedCloseCount = stat->add(Mdb::DeviceContext::Info_Tcp_UnwaitedCloseCount, 0);
	idleTimeoutCount = stat->add(Mdb::DeviceContext::Info_Tcp_IdleTimeoutCount, 0);
	rxtxTimeoutCount = stat->add(Mdb::DeviceContext::Info_Tcp_RxTxTimeoutCount, 0);
	cipPing1ErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_CipPing1ErrorCount, 0);
	cipPing2ErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_CipPing2ErrorCount, 0);
	otherErrorCount = stat->add(Mdb::DeviceContext::Info_Tcp_OtherErrorCount, 0);

	timer = timers->addTimer<TcpConnection, &TcpConnection::procTimer>(this);
	command = new Command(this);
	setId(id);
}

TcpConnection::~TcpConnection() {
	delete command;
	timers->deleteTimer(timer);
}

bool TcpConnection::isConnected() {
	return state->get() > State_Idle;
}

void TcpConnection::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

void TcpConnection::accept(uint16_t id) {
	LOG_DEBUG(LOG_TCPIP, "accept");
	setId(id);
	state->set(State_Ready);
}

bool TcpConnection::connect(const char *domainName, uint16_t port, Mode mode) {
	LOG_DEBUG(LOG_TCPIP, "connect");
	if(state->get() != State_Idle) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Wrong state " << state->get());
		wrongStateCount->inc();
		return false;
	}
	this->serverDomainName = domainName;
	this->serverPort = port;
	this->serverMode = mode;
	this->recvDataFlag = false;
	this->tryCount = GSM_TCP_TRY_MAX_NUMBER;
	gotoStateGprsCheck();
	return true;
}

bool TcpConnection::hasRecvData() {
	return this->recvDataFlag;
}

bool TcpConnection::send(const uint8_t *data, uint32_t dataLen) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "send");
	if(state->get() != State_Ready) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Wrong state " << state->get());
		wrongStateCount->inc();
		return false;
	}
	if(dataLen <= 0) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Wrong data len " << dataLen);
		otherErrorCount->inc();
		return false;
	}
	this->data = (uint8_t*)data;
	this->dataLen = dataLen;
	this->procLen = 0;
	this->packetLen = dataLen > PACKET_SIZE ? PACKET_SIZE : dataLen;
	gotoStateSend();
	return true;
}

bool TcpConnection::recv(uint8_t *buf, uint32_t bufSize) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "recv " << bufSize);
	REMOTE_LOG(RLOG_ECL, "recv\r\n");
	if(state->get() != State_Ready) {
		wrongStateCount->inc();
		return false;
	}
	this->data = buf;
	this->dataSize = bufSize;
	this->procLen = 0;
	if(this->recvDataFlag == false) {
		gotoStateRecvWait();
		return true;
	} else {
		recvDataFlag = false;
		gotoStateRecv();
		return true;
	}
}

void TcpConnection::close() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "close");
	REMOTE_LOG(RLOG_ECL, "close\r\n");
	if(state->get() == State_Idle) {
		wrongStateCount->inc();
		gotoStateFakeClose();
		return;
	}
	gotoStateClose();
}

void TcpConnection::procTimer() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "procTimer " << state->get());
	switch(state->get()) {
	case State_GprsWait: stateGprsWaitTimeout(); break;
	case State_OpenWait: stateOpenWaitTimeout(); break;
	case State_Ready: stateReadyTimeout(); break;
	case State_Send: stateSendTimeout(); break;
	case State_RecvWait: stateRecvWaitTimeout(); break;
	case State_Recv: stateRecvTimeout(); break;
	case State_FakeClose: stateFakeCloseTimeout(); break;
	default: LOG_ERROR(LOG_TCPIP, "#" << id << "Unwaited timeout: " << state->get());
	}
}

void TcpConnection::procResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "procResponse " << state->get());
	switch(state->get()) {
	case State_GprsCheck: stateGprsCheckResponse(result, data); break;
	case State_Ssl: stateSslResponse(result); break;
	case State_Open: stateOpenResponse(result); break;
	case State_Ping: statePingResponse(result); break;
	case State_Send: stateSendResponse(result); break;
	case State_SendClose: stateSendCloseResponse(); break;
	case State_Recv: stateRecvResponse(result, data); break;
	case State_RecvClose: stateRecvCloseResponse(); break;
	case State_Close: stateCloseResponse(); break;
	default: LOG_ERROR(LOG_TCPIP, "#" << id << "Unwaited response: " << state->get() << "," << result);
	}
}

void TcpConnection::procEvent(const char *data) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "procEvent " << state->get());
	switch(state->get()) {
	case State_OpenWait: stateOpenWaitEvent(data); break;
	case State_Ready: stateReadyEvent(data); break;
	case State_Send: stateSendEvent(data); break;
	case State_RecvWait: stateRecvWaitEvent(data); break;
	case State_Recv: stateRecvEvent(data); break;
	default:;
	}
}

void TcpConnection::proc(Event *event) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "proc" << state->get());
	switch(state->get()) {
	case State_GprsWait: stateGprsWaitEvent(event); break;
	default:;
	}
}

void TcpConnection::setId(uint16_t id) {
	this->id = id;
	this->eventPrefix.clear();
	this->eventPrefix << id << ",";
}

bool TcpConnection::isEvent(const char *data, const char *expected) {
	StringParser parser(data, strlen(data));
	if(parser.compareAndSkip(eventPrefix.getString()) == false) {
		return false;
	}
	parser.skipSpace();
	return parser.compare(expected);
}

bool TcpConnection::isEventConnectionClosed(const char *data) {
	return isEvent(data, "CLOSED");
}

bool TcpConnection::isEventRecvData(const char *data) {
	StringParser parser(data, strlen(data));
	if(parser.compareAndSkip("+CIPRXGET:") == false) {
		return false;
	}
	parser.skipSpace();
	if(parser.compareAndSkip("1,") == false) {
		return false;
	}
	uint16_t recvId;
	if(parser.getNumber(&recvId) == false) {
		return false;
	}
	return recvId == id;
}

void TcpConnection::gotoStateIdle() {
	state->set(State_Idle);
}

void TcpConnection::gotoStateGprsCheck() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateGprsCheck");
	command->set(Command::Type_CipStatus, "AT+CIPSTATUS");
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "sendCommand failed");
		executeErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
	state->set(State_GprsCheck);
}

void TcpConnection::stateGprsCheckResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateGprsCheckResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Error code " << result);
		cipStatusErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
	if(strcmp("IP STATUS", data) != 0 && strcmp("IP PROCESSING", data) != 0) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Unwaited answer " << data);
		gprsErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
#ifdef ERP_SSL_OFF
	gotoStateOpen();
#else
	gotoStateSsl();
#endif
}

void TcpConnection::gotoStateGprsWait(uint32_t line) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateGprsWait");
	if(tryCount == 0) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Too many tries.");
		stateOpenError(line);
		return;
	}
	tryCount--;
	commandProcessor->reset();
	timer->start(GSM_RESTART_TIMEOUT);
	state->set(State_GprsWait);
}

void TcpConnection::stateGprsWaitEvent(Event *event) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateGprsWaitEvent");
	switch(event->getType()) {
	case Gsm::Driver::Event_NetworkUp: {
		LOG_DEBUG(LOG_TCPIP,"#" << id <<  "Event_NetworkUp");
		timer->stop();
#ifdef ERP_SSL_OFF
		gotoStateOpen();
#else
		gotoStateSsl();
#endif
		return;
	}
	}
}

void TcpConnection::stateGprsWaitTimeout() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateGprsWaitTimeout");
	gotoStateGprsWait(__LINE__);
}

void TcpConnection::gotoStateSsl() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateSsl");
	if(serverMode == Mode_TcpIpOverSsl) {
		command->set(Command::Type_Result, "AT+CIPSSL=1");
	} else {
		command->set(Command::Type_Result, "AT+CIPSSL=0");
	}
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "sendCommand failed");
		executeErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
	state->set(State_Ssl);
}

void TcpConnection::stateSslResponse(Command::Result result) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateSslResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Ssl failed " << result);
		cipSslErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
	gotoStateOpen();
}

void TcpConnection::gotoStateOpen() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateOpen");
	command->set(Command::Type_Result);
	command->setText() << "AT+CIPSTART=" << id << ",\"TCP\",\"" << serverDomainName << "\",\"" << serverPort << "\"";
#ifdef ERP_SSL_OFF
	if(commandProcessor->execute(command) == false) {
#else
	if(commandProcessor->executeOutOfTurn(command) == false) {
#endif
		LOG_ERROR(LOG_TCPIP, "#" << id << "cipstart failed");
		executeErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
	state->set(State_Open);
	return;
}

void TcpConnection::stateOpenResponse(Command::Result result) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateOpenResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Open failed " << result);
		cipStartErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
	gotoStateOpenWait();
}

void TcpConnection::gotoStateOpenWait() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateOpenWait");
	timer->start(AT_CIPSTART_TIMEOUT);
	state->set(State_OpenWait);
}

void TcpConnection::stateOpenWaitEvent(const char *data) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateOpenWaitEvent");
	if(isEvent(data, "CONNECT OK") == true || isEvent(data, "ALREADY CONNECT") == true) {
		LOG_INFO(LOG_TCPIP, data);
		timer->stop();
		gotoStateReady();
		Event event(TcpIp::Event_ConnectOk);
		courier.deliver(&event);
		return;
	}
	if(isEvent(data, "CONNECT FAIL") == true) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Error: " << data);
		connectErrorCount->inc();
		gotoStateGprsWait(__LINE__);
		return;
	}
}

// CIPSTART иногда зависает без ответа и требуется перезапуск нижнего уровня.
void TcpConnection::stateOpenWaitTimeout() {
	LOG_ERROR(LOG_TCPIP, "#" << id << "stateOpenWaitTimeout");
	gotoStateGprsWait(__LINE__);
}

void TcpConnection::gotoStatePing() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStatePing2");
	command->set(Command::Type_CipPing);
	command->setText() << "AT+CIPPING=8.8.8.8,1";
	command->setTimeout(10000);
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "cipping failed");
		cipPing2ErrorCount->inc();
		stateOpenError(__LINE__);
		return;
	}
	state->set(State_Ping);
}

void TcpConnection::statePingResponse(Command::Result result) {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "statePing2Response");
	if(result != Command::Result_OK) {
		cipPing2ErrorCount->inc();
		stateOpenError(__LINE__);
		return;
	}
	stateOpenError(__LINE__);
}

void TcpConnection::stateOpenError(uint32_t line) {
	LOG_ERROR(LOG_TCPIP, "#" << id << "stateOpenError");
	timer->stop();
	state->set(State_Idle);
	EventError event(TcpIp::Event_ConnectError, "gtc", line);
	commandProcessor->dump(&(event.trace));
	courier.deliver(&event);
}

void TcpConnection::gotoStateReady() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateReady");
	timer->start(GSM_TCP_IDLE_TIMEOUT);
	state->set(State_Ready);
}

void TcpConnection::stateReadyEvent(const char *data) {
	if(isEventConnectionClosed(data) == true) {
		unwaitedCloseCount->inc();
		state->set(State_Idle);
		Event event(TcpIp::Event_Close);
		courier.deliver(&event);
		return;
	} else if(isEventRecvData(data) == true) {
		recvDataFlag = true;
		Event event(TcpIp::Event_IncomingData);
		courier.deliver(&event);
		return;
	} else {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Wrong event: " << state->get() << "," << data);
		return;
	}
}

void TcpConnection::stateReadyTimeout() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateReadyTimeout");
	idleTimeoutCount->inc();
	gotoStateClose();
}

void TcpConnection::gotoStateSend() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateSend");
	command->set(Command::Type_SendData, "", AT_CIPSEND_TIMEOUT);
	command->setText() << "AT+CIPSEND=" << id << "," << packetLen;
	command->setData(this->data + procLen, this->packetLen);
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Send failed");
		executeErrorCount->inc();
		gotoStateFakeClose();
		return;
	}
	timer->start(GSM_TCP_IDLE_TIMEOUT);
	state->set(State_Send);
}

void TcpConnection::stateSendResponse(Command::Result result) {
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Send failed " << result);
		sendErrorCount->inc();
		gotoStateClose();
		return;
	}

	procLen += packetLen;
	uint16_t tailLen = dataLen - procLen;
	if(tailLen > 0) {
		packetLen = tailLen > PACKET_SIZE ? PACKET_SIZE : tailLen;
		LOG_INFO(LOG_TCPIP, "#" << id << "Send " << packetLen << " from " << tailLen << "/" << dataLen);
		gotoStateSend();
		return;
	} else {
		LOG_INFO(LOG_TCPIP, "#" << id << "Send complete");
		gotoStateReady();
		Event event(TcpIp::Event_SendDataOk);
		courier.deliver(&event);
		return;
	}
}

void TcpConnection::stateSendEvent(const char *data) {
	if(isEventConnectionClosed(data) == true) {
		unwaitedCloseCount->inc();
		gotoStateSendClose();
		return;
	} else if(isEventRecvData(data) == true) {
		recvDataFlag = true;
		return;
	} else {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Unwaited event: " << state->get() << "," << data);
		return;
	}
}

void TcpConnection::stateSendTimeout() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateSendTimeout");
	rxtxTimeoutCount->inc();
	gotoStateSendClose();
}

void TcpConnection::gotoStateSendClose() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateSendClose");
	timer->stop();
	state->set(State_SendClose);
}

void TcpConnection::stateSendCloseResponse() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateSendCloseResponse");
	state->set(State_Idle);
	Event event(TcpIp::Event_Close);
	courier.deliver(&event);
}

void TcpConnection::gotoStateRecvWait() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateRecvWait");
	REMOTE_LOG(RLOG_ECL, "gotoStateRecvWait\r\n");
	timer->start(GSM_TCP_IDLE_TIMEOUT);
	state->set(State_RecvWait);
}

void TcpConnection::stateRecvWaitEvent(const char *data) {
	REMOTE_LOG(RLOG_ECL, "stateRecvWaitEvent: ");
	REMOTE_LOG(RLOG_ECL, data);
	REMOTE_LOG(RLOG_ECL, "\r\n");
	if(isEventConnectionClosed(data) == true) {
		unwaitedCloseCount->inc();
		state->set(State_Idle);
		Event event(TcpIp::Event_Close);
		courier.deliver(&event);
		return;
	} else if(isEventRecvData(data) == true) {
		recvDataFlag = false;
		gotoStateRecv();
		return;
	} else {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Wrong event: " << state->get() << "," << data);
		return;
	}
}

void TcpConnection::stateRecvWaitTimeout() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateRecvWaitTimeout");
	REMOTE_LOG(RLOG_ECL, "stateRecvWaitTimeout\r\n");
	idleTimeoutCount->inc();
	gotoStateClose();
}

void TcpConnection::gotoStateRecv() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateRecv");
	REMOTE_LOG(RLOG_ECL, "gotoStateRecv");
	uint16_t freeLen = dataSize - procLen;
	if(freeLen <= 0) {
		LOG_INFO(LOG_TCPIP, "#" << id << "Not more buffer " << dataSize << "," << procLen);
		gotoStateReady();
		Event event(TcpIp::Event_RecvDataOk, (uint16_t)procLen);
		courier.deliver(&event);
		return;
	}

	uint16_t bufSize = freeLen >= PACKET_SIZE ? PACKET_SIZE : freeLen;
	command->set(Command::Type_RecvData);
	command->setText() << "AT+CIPRXGET=2," << id << "," << bufSize;
	command->setData(data + procLen, bufSize);
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Recv failed");
		executeErrorCount->inc();
		gotoStateFakeClose();
		return;
	}

	timer->start(GSM_TCP_IDLE_TIMEOUT);
	state->set(State_Recv);
}

void TcpConnection::stateRecvResponse(Command::Result result, const char *data) {
	REMOTE_LOG(RLOG_ECL, "stateRecvResponse: ");
	if(data != NULL) { REMOTE_LOG(RLOG_ECL, data); }
	REMOTE_LOG(RLOG_ECL, "\r\n");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Recv failed " << result);
		recvErrorCount->inc();
		gotoStateClose();
		return;
	}

	if(parseRecvResponse(data) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Parse failed " << result);
		otherErrorCount->inc();
		gotoStateClose();
		return;
	}

	procLen += packetLen;
	if(dataLen > 0) {
		recvDataFlag = true;
		gotoStateRecv();
		return;
	} else {
		LOG_INFO(LOG_TCPIP, "#" << id << "No more data");
		gotoStateReady();
		Event event(TcpIp::Event_RecvDataOk, (uint16_t)procLen);
		courier.deliver(&event);
		return;
	}
}

bool TcpConnection::parseRecvResponse(const char *data) {
	StringParser parser(data);
	if(parser.compareAndSkip("+CIPRXGET:") == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Unwaited data: " << data);
		return false;
	}
	parser.skipSpace();
	uint16_t mode = 0;
	if(parser.getNumber(&mode) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Not found first parameter <" << data << ">");
		return false;
	}
	parser.skipEqual(" ,");
	uint16_t recvId = 0;
	if(parser.getNumber(&recvId) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Not found second parameter <" << data << ">");
		return false;
	}
	parser.skipEqual(" ,");
	if(parser.getNumber(&packetLen) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Not found third parameter <" << data << ">");
		return false;
	}
	parser.skipEqual(" ,");
	if(parser.getNumber(&dataLen) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Not found fourth parameter <" << data << ">");
		return false;
	}
	LOG_INFO(LOG_TCPIP, "#" << id << "parsed CIPRXGET: " << packetLen << "," << dataLen);
	return true;
}

void TcpConnection::stateRecvEvent(const char *data) {
	REMOTE_LOG(RLOG_ECL, "stateRecvEvent: ");
	if(data != NULL) { REMOTE_LOG(RLOG_ECL, data); }
	REMOTE_LOG(RLOG_ECL, "\r\n");
	if(isEventConnectionClosed(data) == true) {
		unwaitedCloseCount->inc();
		gotoStateRecvClose();
		return;
	} else if(isEventRecvData(data) == true) {
		recvDataFlag = true;
		return;
	}
}

void TcpConnection::stateRecvTimeout() {
	REMOTE_LOG(RLOG_ECL, "stateRecvTimeout\r\n");
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateRecvTimeout");
	rxtxTimeoutCount->inc();
	gotoStateRecvClose();
}

void TcpConnection::gotoStateRecvClose() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateRecvClose");
	timer->stop();
	state->set(State_RecvClose);
}

void TcpConnection::stateRecvCloseResponse() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateRecvCloseResponse");
	state->set(State_Idle);
	Event event(TcpIp::Event_Close);
	courier.deliver(&event);
}

void TcpConnection::gotoStateClose() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateClose");
	command->set(Command::Type_CipClose);
	command->setText() << "AT+CIPCLOSE=" << id << ",0";
	if(commandProcessor->execute(command) == false) {
		LOG_ERROR(LOG_TCPIP, "#" << id << "Close failed");
		gotoStateFakeClose();
		return;
	}
	timer->stop();
	state->set(State_Close);
}

void TcpConnection::stateCloseResponse() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateCloseResponse");
	gotoStateIdle();
	Event event(TcpIp::Event_Close);
	courier.deliver(&event);
}

void TcpConnection::gotoStateFakeClose() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "gotoStateFakeClose");
	timer->start(FAKE_CLOSE_TIMEOUT);
	state->set(State_FakeClose);
}

void TcpConnection::stateFakeCloseTimeout() {
	LOG_DEBUG(LOG_TCPIP, "#" << id << "stateFakeCloseTimeout");
	gotoStateIdle();
	Event event(TcpIp::Event_Close);
	courier.deliver(&event);
}

}
