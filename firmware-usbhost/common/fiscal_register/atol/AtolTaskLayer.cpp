#include "AtolTaskLayer.h"

#include "logger/include/Logger.h"

namespace Atol {

TaskLayer::TaskLayer(TimerEngine *timers, PacketLayerInterface *packetLayer) :
	timers(timers),
	packetLayer(packetLayer),
	observer(NULL),
	state(State_Idle),
	task(ATOL_PACKET_MAX_SIZE),
	taskId(ATOL_TASK_ID_MIN_NUMBER)
{
	this->packetLayer->setObserver(this);
	this->timer = timers->addTimer<TaskLayer, &TaskLayer::procTimer>(this);
}

void TaskLayer::setObserver(TaskLayerObserver *observer) {
	this->observer = observer;
}

bool TaskLayer::connect(const char *domainname, uint16_t port, TcpIp::Mode mode) {
	LOG_INFO(LOG_FRP, "connect");
	if(state != State_Idle) {
		LOG_ERROR(LOG_FRP, "Wrong state " << state);
		return false;
	}

	this->tryNumber = 0;
	this->ipaddr = domainname;
	this->port = port;
	this->mode = mode;
	gotoStateConnect();
	return true;
}

bool TaskLayer::sendRequest(const uint8_t *data, const uint16_t dataLen) {
	LOG_INFO(LOG_FRP, "sendRequest");
	if(state != State_Wait) {
		LOG_ERROR(LOG_FRP, "Wrong state " << state);
		return false;
	}
	sendData = data;
	sendDataLen = dataLen;
	gotoStateTaskAdd();
	return true;
}

bool TaskLayer::disconnect() {
	LOG_INFO(LOG_FRP, "disconnect");
	if(packetLayer->disconnect() == false) {
		LOG_ERROR(LOG_FRP, "Disconnect failed");
		state = State_Idle;
		return false;
	}
	state = State_Disconnect;
	return true;
}

void TaskLayer::procRecvData(uint8_t packetId, const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "procRecvData " << packetId);
	LOG_TRACE_HEX(LOG_FRP, data, dataLen);
	switch(state) {
	case State_Init: stateInitResponse(packetId, data, dataLen); return;
	case State_TaskAdd: stateTaskAddResponse(packetId, data, dataLen); return;
	case State_TaskAsync: stateTaskAsyncResponse(packetId, data, dataLen); return;
	case State_TaskReq: stateTaskReqResponse(packetId, data, dataLen); return;
	default: LOG_ERROR(LOG_FRP, "Unwaited response " << state);
	}
}

void TaskLayer::procError(PacketLayerObserver::Error error) {
	LOG_DEBUG(LOG_FRP, "procError " << state << "," << error);
	switch(state) {
	case State_Connect: stateConnectProcError(error); return;
	case State_InitWait: stateInitError(error); return;
	case State_Init: stateInitError(error); return;
	case State_TaskAdd: stateTaskAddError(error); return;
	case State_TaskAsync: stateTaskAddError(error); return;
	case State_TackReconnect: stateTaskReconnectError(error); return;
	case State_Disconnect: {
		state = State_Idle;
		observer->procError(TaskLayerObserver::Error_OK);
		return;
	}
	default: {
		switch(error) {
		case PacketLayerObserver::Error_RemoteClose: procError(TaskLayerObserver::Error_RemoteClose); return;
		case PacketLayerObserver::Error_SendFailed: procError(TaskLayerObserver::Error_PacketSendFailed); return;
		case PacketLayerObserver::Error_RecvFailed: procError(TaskLayerObserver::Error_PacketRecvFailed); return;
		case PacketLayerObserver::Error_RecvTimeout: procError(TaskLayerObserver::Error_PacketTimeout); return;
		default: procError(TaskLayerObserver::Error_UnknownError);
		}
	}
	}
}

void TaskLayer::procTimer() {
	LOG_DEBUG(LOG_FRP, "procTimer");
	switch(state) {
	case State_InitWait: stateInitWaitTimeout(); break;
	case State_TaskAsync: stateTaskAsyncTimeout(); break;
	case State_ReconnectPause: stateReconnectPauseTimeout(); break;
	default: LOG_ERROR(LOG_FRP, "Unwaited timeout " << state);
	}
}

void TaskLayer::gotoStateConnect() {
	LOG_INFO(LOG_FRP, "gotoStateConnect");
	tryNumber++;
	if(tryNumber > ATOL_TASK_TRY_NUMBER) {
		procError(TaskLayerObserver::Error_ConnectFailed);
		return;
	}
	packetLayer->connect(ipaddr, port, mode);
	state = State_Connect;
}

void TaskLayer::stateConnectProcError(PacketLayerObserver::Error error) {
	LOG_INFO(LOG_FRP, "stateConnectProcError");
	switch(error) {
	case PacketLayerObserver::Error_OK: {
		gotoStateInitWait();
		return;
	}
	case PacketLayerObserver::Error_ConnectFailed: {
		LOG_INFO(LOG_FRP, "connection failed");
		gotoStateReconnectPause();
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited response " << state << "," << error); return;
	}
}

void TaskLayer::gotoStateInitWait() {
	LOG_DEBUG(LOG_FRP, "gotoStateInitWait");
	timer->start(ATOL_TASK_INIT_DELAY);
	state = State_InitWait;
}

void TaskLayer::stateInitWaitTimeout() {
	LOG_DEBUG(LOG_FRP, "stateInitWaitTimeout");
	gotoStateInit();
}

void TaskLayer::gotoStateInit() {
	LOG_DEBUG(LOG_FRP, "gotoStateInit");
	TaskAbortHeader *req = (TaskAbortHeader*)task.getData();
	req->command = TaskCommand_Abort;
	task.setLen(sizeof(TaskAbortHeader));
	packetLayer->sendPacket(task.getData(), task.getLen());
	state = State_Init;
}

void TaskLayer::stateInitResponse(uint8_t packetId, const uint8_t *data, const uint16_t dataLen) {
	LOG_INFO(LOG_FRP, "stateInitResponse " << packetId);
	if(sizeof(TaskResponse) > dataLen) {
		LOG_ERROR(LOG_FRP, "Wrong response size " << dataLen);
		procError(TaskLayerObserver::Error_PacketWrongSize);
		return;
	}
	TaskResponse *resp = (TaskResponse*)data;
	switch(resp->result) {
	case TaskStatus_Result: {
		LOG_DEBUG_HEX(LOG_FRP, data, dataLen);
		state = State_Wait;
		observer->procError(TaskLayerObserver::Error_OK);
		return;
	}
	default:
		LOG_DEBUG(LOG_FRP, "Unwaited result=" << resp->result << ",state=" << state);
		procError(TaskLayerObserver::Error_TaskFailed);
		return;
	}
}

void TaskLayer::stateInitError(PacketLayerObserver::Error error) {
	LOG_INFO(LOG_FRP, "stateInitError");
	switch(error) {
	case PacketLayerObserver::Error_RemoteClose: {
		LOG_INFO(LOG_FRP, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>RECONNECT " << tryNumber);
		gotoStateReconnectPause();
		return;
	}
	case PacketLayerObserver::Error_SendFailed: procError(TaskLayerObserver::Error_PacketSendFailed); return;
	case PacketLayerObserver::Error_RecvFailed: procError(TaskLayerObserver::Error_PacketRecvFailed); return;
	case PacketLayerObserver::Error_RecvTimeout: procError(TaskLayerObserver::Error_PacketTimeout); return;
	default: procError(TaskLayerObserver::Error_UnknownError);
	}
}

void TaskLayer::gotoStateTaskAdd() {
	incTaskId();
	LOG_INFO(LOG_FRP, "gotoTaskAdd");
	TaskAddHeader *req = (TaskAddHeader*)task.getData();
	req->command = TaskCommand_Add;
	req->flags = TaskFlag_NeedResult | TaskFlag_IgnoreError;
	req->tid = taskId;
	for(uint16_t i = 0; i < sendDataLen; i++) {
		req->data[i] = sendData[i];
	}
	task.setLen(sizeof(TaskAddHeader) + sendDataLen);
	packetLayer->sendPacket(task.getData(), task.getLen());
	state = State_TaskAdd;
}

void TaskLayer::stateTaskAddResponse(uint8_t packetId, const uint8_t *data, const uint16_t dataLen) {
	LOG_INFO(LOG_FRP, "stateTaskAddResponse " << packetId << "," << taskId);
	if(sizeof(TaskResponse) > dataLen) {
		LOG_ERROR(LOG_FRP, "Wrong response size " << dataLen);
		procError(TaskLayerObserver::Error_PacketWrongSize);
		return;
	}
	TaskResponse *resp = (TaskResponse*)data;
	LOG_INFO(LOG_FRP, "result " << resp->result);
	switch(resp->result) {
	case TaskStatus_Result:
	case TaskStatus_Error: {
		state = State_Wait;
		observer->procRecvData(resp->data, dataLen - sizeof(TaskResponse));
		return;
	}
	case TaskStatus_Pending:
	case TaskStatus_Waiting:
	case TaskStatus_InProgress: gotoStateTaskAsync(); return;
	case TaskStatus_Stopped: stateTaskAddResponseStopped(data, dataLen); return;
	default:
		LOG_DEBUG(LOG_FRP, "Unwaited result=" << resp->result << ",state=" << state);
		procError(TaskLayerObserver::Error_TaskFailed);
		return;
	}
}

void TaskLayer::stateTaskAddResponseStopped(const uint8_t *data, const uint16_t dataLen) {
	LOG_INFO(LOG_FRP, "stateTaskAddResponseStopped");
	if(sizeof(TaskStoppedResponse) > dataLen) {
		LOG_ERROR(LOG_FRP, "Wrong response size " << dataLen);
		procError(TaskLayerObserver::Error_PacketWrongSize);
		return;
	}
	TaskStoppedResponse *resp = (TaskStoppedResponse*)data;
	LOG_INFO(LOG_FRP, "tid " << resp->tid);
	observer->procError(TaskLayerObserver::Error_TaskFailed);
}

void TaskLayer::stateTaskAddError(PacketLayerObserver::Error error) {
	LOG_INFO(LOG_FRP, "stateTaskAddError");
	switch(error) {
	case PacketLayerObserver::Error_RemoteClose: {
		LOG_INFO(LOG_FRP, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>RECONNECT");
		gotoStateTaskReconnect();
		return;
	}
	case PacketLayerObserver::Error_SendFailed: procError(TaskLayerObserver::Error_PacketSendFailed); return;
	case PacketLayerObserver::Error_RecvFailed: procError(TaskLayerObserver::Error_PacketRecvFailed); return;
	case PacketLayerObserver::Error_RecvTimeout: procError(TaskLayerObserver::Error_PacketTimeout); return;
	default: procError(TaskLayerObserver::Error_UnknownError);
	}
}

void TaskLayer::gotoStateTaskAsync() {
	LOG_DEBUG(LOG_FRP, "gotoStateTaskAsync");
	timer->start(ATOL_TASK_ASYNC_TIMEOUT);
	state = State_TaskAsync;
}

void TaskLayer::stateTaskAsyncResponse(uint8_t packetId, const uint8_t *data, const uint16_t dataLen) {
	LOG_INFO(LOG_FRP, "stateTaskAsyncResponse " << packetId);
	timer->stop();
	if(sizeof(TaskAsyncResult) > dataLen) {
		LOG_ERROR(LOG_FRP, "Wrong response size " << dataLen);
		procError(TaskLayerObserver::Error_PacketWrongSize);
		return;
	}
	TaskAsyncResult *resp = (TaskAsyncResult*)data;
	LOG_INFO(LOG_FRP, "result " << resp->result);
	switch(resp->result) {
	case TaskStatus_AsyncError: //FIRE после возникновения этой ошибки требуется перезапуск очереди
	case TaskStatus_AsyncResult: {
		if(resp->tid != taskId) {
			LOG_INFO(LOG_FRP, "Wrong TID " << resp->tid << "<>" << taskId);
			observer->procError(TaskLayerObserver::Error_TaskFailed);
			return;
		}
		state = State_Wait;
		observer->procRecvData(resp->data, dataLen - sizeof(TaskAsyncResult));
		return;
	}
	default: {
		LOG_INFO(LOG_FRP, "error");
		observer->procError(TaskLayerObserver::Error_TaskFailed);
		return;
	}
	}
}

void TaskLayer::stateTaskAsyncTimeout() {
	LOG_INFO(LOG_FRP, "stateTaskAsyncTimeout");
	observer->procError(TaskLayerObserver::Error_TaskFailed);
}

void TaskLayer::gotoStateTaskReq() {
	LOG_INFO(LOG_FRP, "gotoStateTaskReq " << taskId);
	TaskReqRequest *req = (TaskReqRequest*)task.getData();
	req->command = TaskCommand_Req;
	req->tid = taskId;
	task.setLen(sizeof(TaskReqRequest));
	packetLayer->sendPacket(task.getData(), task.getLen());
	state = State_TaskReq;
}

void TaskLayer::stateTaskReqResponse(const uint8_t packetId, const uint8_t *data, const uint16_t dataLen) {
	LOG_INFO(LOG_FRP, "stateTaskReqResponse " << packetId << "," << taskId);
	if(sizeof(TaskResponse) > dataLen) {
		LOG_ERROR(LOG_FRP, "Wrong response size " << dataLen);
		procError(TaskLayerObserver::Error_PacketWrongSize);
		return;
	}
	TaskResponse *resp = (TaskResponse*)data;
	LOG_INFO(LOG_FRP, "result " << resp->result);
	switch(resp->result) {
	case TaskStatus_Result:
	case TaskStatus_Error: {
		state = State_Wait;
		observer->procRecvData(resp->data, dataLen - sizeof(TaskResponse));
		return;
	}
	case TaskStatus_Pending:
	case TaskStatus_Waiting:
	case TaskStatus_InProgress: {
		gotoStateTaskAsync();
		return;
	}
	case TaskStatus_Stopped: {
		stateTaskAddResponseStopped(data, dataLen);
		return;
	}
	case Atol::Error_NotFound: {
		LOG_ERROR(LOG_FRP, ">>>>>>>>>>>>>>>>>TID NOT FOUND");
		gotoStateTaskAdd();
		return;
	}
	default:
		LOG_DEBUG(LOG_FRP, "Unwaited result=" << resp->result << ",state=" << state);
		procError(TaskLayerObserver::Error_TaskFailed);
		return;
	}
}

void TaskLayer::gotoStateTaskReconnect() {
	LOG_DEBUG(LOG_FRP, "gotoStateTaskReconnect");
	tryNumber++;
	if(tryNumber > ATOL_TASK_TRY_NUMBER) {
		procError(TaskLayerObserver::Error_ConnectFailed);
		return;
	}
	packetLayer->connect(ipaddr, port, mode);
	state = State_TackReconnect;
}

void TaskLayer::stateTaskReconnectError(PacketLayerObserver::Error error) {
	LOG_DEBUG(LOG_FRP, "stateTaskReconnectError");
	switch(error) {
	case PacketLayerObserver::Error_OK: {
		LOG_INFO(LOG_FRP, "Connect succeed");
		gotoStateTaskReq();
		return;
	}
	case PacketLayerObserver::Error_ConnectFailed: {
		LOG_INFO(LOG_FRP, "Connect failed");
		state = State_Idle;
		observer->procError(TaskLayerObserver::Error_RemoteClose);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited response " << state << "," << error); return;
	}
}

void TaskLayer::gotoStateReconnectPause() {
	LOG_DEBUG(LOG_FRP, "gotoStateReconnectPause");
	tryNumber++;
	if(tryNumber > ATOL_TASK_TRY_NUMBER) {
		procError(TaskLayerObserver::Error_ConnectFailed);
		return;
	}
	timer->start(ATOL_TASK_RECONNECT_TIMEOUT);
	state = State_ReconnectPause;
}

void TaskLayer::stateReconnectPauseTimeout() {
	LOG_DEBUG(LOG_FRP, "stateReconnectPauseTimeout");
	gotoStateConnect();
}

void TaskLayer::procError(TaskLayerObserver::Error error) {
	LOG_INFO(LOG_FRP, "procError " << error);
	state = State_Idle;
	observer->procError(error);
}

void TaskLayer::incTaskId() {
	taskId++;
	if(taskId > ATOL_TASK_ID_MAX_NUMBER) {
		taskId = ATOL_TASK_ID_MIN_NUMBER;
	}
}

}
