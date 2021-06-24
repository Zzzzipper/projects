#include "CciFrankeCommandLayer.h"

#include "common/ccicsi/CciCsiProtocol.h"
#include "common/mdb/slave/cashless/MdbSlaveCashless3.h"
#include "common/utils/include/DecimalPoint.h"
#include "common/utils/include/CodePage.h"
#include "common/logger/include/Logger.h"

#include <string.h>

#define FRANKE_APPROVING_TIMEOUT 120

using namespace CciCsi;

namespace Cci {
namespace Franke {

CommandLayer::CommandLayer(PacketLayerInterface *packetLayer, TimerEngine *timers, EventEngineInterface *eventEngine) :
	packetLayer(packetLayer),
	timers(timers),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle),
	response(CCICSI_PACKET_MAX_SIZE)
{
//	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
//	this->context->setState(State_Idle);
	this->packetLayer->setObserver(this);
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
}

CommandLayer::~CommandLayer() {
	timers->deleteTimer(this->timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::disableProducts() {
	productState = 0xFFFF;
}

void CommandLayer::enableProducts() {
	productState = 0;
}

void CommandLayer::reset() {
	LOG_INFO(LOG_CCI, "reset");
	packetLayer->reset();
	state = State_Wait;
}

bool CommandLayer::isInited() {
	return false;
}

bool CommandLayer::isEnable() {
	return false;
}

void CommandLayer::setCredit(uint32_t credit) {
	LOG_INFO(LOG_CCI, "setCredit");
	if(state == State_Wait) {
		this->credit = credit;
		this->state = State_Credit;
	}
}

void CommandLayer::approveVend(uint32_t ) {
	LOG_INFO(LOG_CCI, "approveVend");
	if(state == State_Approving) {
		if(operation == Operation_B) {
			sendBApproved();
		} else {
			sendInquiryApproved();
		}
		state = State_Vending;
	}
}

void CommandLayer::denyVend(bool close) {
	LOG_INFO(LOG_CCI, "denyVend");
	if(state == State_Approving) {
		if(operation == Operation_B) {
			sendBDenied();
		} else {
			sendInquiryDenied();
		}
		state = State_Wait;
	}
}

void CommandLayer::cancelVend() {
	LOG_INFO(LOG_CCI, "cancelVend");
	state = State_Wait;
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_CCI, "procTimer");

}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "procPacket");
	LOG_DEBUG_HEX(LOG_CCI, data, dataLen);
	switch(state) {
	case State_Wait: stateWaitRequest(data, dataLen); break;
	case State_Credit: stateCreditRequest(data, dataLen); break;
	case State_Approving: stateApprovingRequest(data, dataLen); break;
	case State_Vending: stateVendingRequest(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited data " << state);
	}
}

void CommandLayer::stateWaitRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateWaitRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: sendStatusReady(); break;
	case Operation_BillingEnable: sendBillingEnable(); break;
	case Operation_Buttons: sendButtons(); break;
	case Operation_MachineMode: sendMachineMode(); break;
	case Operation_Telemetry: sendTelemetry(); break;
	case Operation_Inquiry: stateWaitOperationInquiry(data, dataLen); break;
	case Operation_C: sendC(); break;
	case Operation_B: stateWaitOperationB(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateWaitOperationInquiry(const uint8_t *data, uint16_t ) {
	InquiryRequest *req = (InquiryRequest*)data;
	LOG_DEBUG(LOG_CCI, "sendB " << req->articleNumber.get() << "," << req->exec.get());
	operation = Operation_Inquiry;
	productId = req->articleNumber.get();

	gotoStateApproving();
	MdbSlaveCashlessInterface::EventVendRequest event(deviceId, MdbSlaveCashlessInterface::Event_VendRequest, productId, 0);
	eventEngine->transmit(&event);
}

void CommandLayer::stateWaitOperationB(const uint8_t *data, uint16_t ) {
	AmountRequest *req = (AmountRequest*)data;
	LOG_DEBUG(LOG_CCI, "sendB " << req->articleNumber.get() << "," << req->exec.get());
	operation = Operation_B;
	productId = req->articleNumber.get();

	gotoStateApproving();
	MdbSlaveCashlessInterface::EventVendRequest event(deviceId, MdbSlaveCashlessInterface::Event_VendRequest, productId, 0);
	eventEngine->transmit(&event);
}

void CommandLayer::stateCreditRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateCreditRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: sendStatusReady(); break;
	case Operation_BillingEnable: sendBillingEnable(); break;
	case Operation_Buttons: sendButtons(); break;
	case Operation_MachineMode: sendMachineMode(); break;
	case Operation_Telemetry: sendTelemetry(); break;
	case Operation_Inquiry: stateCreditOperationInquiry(data, dataLen); break;
	case Operation_C: sendC(); break;
	case Operation_B: stateCreditOperationB(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateCreditOperationInquiry(const uint8_t *data, uint16_t ) {
	InquiryRequest *req = (InquiryRequest*)data;
	LOG_DEBUG(LOG_CCI, "sendB " << req->articleNumber.get() << "," << req->exec.get());
	operation = Operation_Inquiry;
	productId = req->articleNumber.get();

	gotoStateApproving();
	MdbSlaveCashlessInterface::EventVendRequest event(deviceId, MdbSlaveCashlessInterface::Event_VendRequest, productId, 0);
	eventEngine->transmit(&event);
}

void CommandLayer::stateCreditOperationB(const uint8_t *data, uint16_t ) {
	AmountRequest *req = (AmountRequest*)data;
	LOG_DEBUG(LOG_CCI, "sendB " << req->articleNumber.get() << "," << req->exec.get());
	operation = Operation_B;
	productId = req->articleNumber.get();

	gotoStateApproving();
	MdbSlaveCashlessInterface::EventVendRequest event(deviceId, MdbSlaveCashlessInterface::Event_VendRequest, productId, 0);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateApproving() {
	LOG_DEBUG(LOG_CCI, "gotoStateApproving");
	statusCount = 0;
	state = State_Approving;
}

void CommandLayer::stateApprovingRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateApprovingRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateVendingRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateVendingRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: stateVendingRequestStatus(); break;
	case Operation_BillingEnable: sendBillingEnable(); break;
	case Operation_Buttons: sendButtons(); break;
	case Operation_MachineMode: sendMachineMode(); break;
	case Operation_Telemetry: sendTelemetry(); break;
	case Operation_Inquiry: sendInquiryDenied(); break;
	case Operation_C: sendC(); break;
	case Operation_B: sendBDenied(); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateVendingRequestStatus() {
	sendStatusReady();
	state = State_Wait;
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendComplete);
	eventEngine->transmit(&event);
}

/* Команда Identification
void Franke::X_Handler()
{
    m_sendBuf[0] = CM_STX;
    m_sendBuf[1] = 'X';
    m_sendBuf[2] = 0x32; Interface_CCI
    m_sendBuf[3] = 0x36; T1_MdbCoinChanger
    m_sendBuf[4] = 0x30;
    m_sendBuf[5] = 0x32; 207
    m_sendBuf[6] = 0x30;
    m_sendBuf[7] = 0x37;
    m_sendBuf[8] = 0x30;
    m_sendBuf[9] = 0x34;

    m_parser->addCRC(m_sendBuf, 10);
    m_parser->sendData(m_sendBuf, 14);

    //debug("X handler\r\n");
}
 */
void CommandLayer::sendIdentification() {
	IdentificationResponse *resp = (IdentificationResponse*)response.getData();
	resp->type = Operation_Identification;
#if 0
	resp->interface = Interface_CSI;
	resp->paymentSystem.set(T1_MdbCoinChanger);
	resp->version.set(207);
	resp->level.set(4);
#else
	resp->interface = Interface_CCI;
	resp->paymentSystem.set(T1_MdbCoinChanger);
	resp->version.set(207);
	resp->level.set(4);
#endif
	response.setLen(sizeof(IdentificationResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendButtons() {
	LOG_DEBUG(LOG_CCI, "sendButtons " << productState);
	CciCsi::ButtonsResponse *resp = (CciCsi::ButtonsResponse*)response.getData();
	resp->type = CciCsi::Operation_Buttons;
	resp->magic1 = '9';
	resp->state.set(productState);
	resp->magic2[0] = '0';
	resp->magic2[1] = '0';
	resp->magic2[2] = '0';
	resp->magic3 = '0';
	response.setLen(sizeof(CciCsi::ButtonsResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendBillingEnable() {
	LOG_DEBUG(LOG_CCI, "sendBillingEnable");

}

/* Команда Machine Mode
void Franke::M_Handler()
{
    m_sendBuf[0] = CM_STX;
    m_sendBuf[1] = 'M';
    m_sendBuf[2] = 0x80;
    m_sendBuf[3] = 0x80;

    m_parser->addCRC(m_sendBuf, 4);
    m_parser->sendData(m_sendBuf, 8);

    //debug("M handler\r\n");
}
*/
void CommandLayer::sendMachineMode() {
	LOG_DEBUG(LOG_CCI, "sendMachineMode");
	CciCsi::MachineModeResponse *resp = (CciCsi::MachineModeResponse*)response.getData();
	resp->type = CciCsi::Operation_MachineMode;
	resp->magic1 = 0x80;
	resp->magic2 = 0x80;
	response.setLen(sizeof(CciCsi::MachineModeResponse));
	packetLayer->sendPacket(&response);
}

/* Данные телеметрии, события
void Franke::D_Handler()
{
    m_sendBuf[0] = CM_STX;
    m_sendBuf[1] = 'D';
    m_sendBuf[2] = '1';

    m_parser->addCRC(m_sendBuf, 3);
    m_parser->sendData(m_sendBuf, 7);

    if (m_cciCommand.data[0] == '2') {
        m_operator->recvEventPackage(m_cciCommand.data, m_cciCommand.len);
    }
    else if (m_cciCommand.data[0] == '3') {
        m_operator->recvDataPackage(m_cciCommand.data, m_cciCommand.len);
    }

    //debug("D handler\r\n");
}
 */
void CommandLayer::sendTelemetry() {
	LOG_DEBUG(LOG_CCI, "sendTelemetry");
	CciCsi::TelemetryResponse *resp = (CciCsi::TelemetryResponse*)response.getData();
	resp->type = CciCsi::Operation_Telemetry;
	resp->magic1 = '1';
	response.setLen(sizeof(CciCsi::TelemetryResponse));
	packetLayer->sendPacket(&response);
}

/*
void Franke::C_Handler()
{
    m_sendBuf[0] = CM_STX;
    m_sendBuf[1] = 'C';
    m_sendBuf[2] = 0x30;
    m_sendBuf[3] = 0x30;
    m_sendBuf[4] = 0x30;
    m_sendBuf[5] = 0x30;
    m_sendBuf[6] = 0x30;
    m_sendBuf[7] = 0x30;
    m_sendBuf[8] = '0'; // Точка отсутствует

    m_parser->addCRC(m_sendBuf, 9);
    m_parser->sendData(m_sendBuf, 13);

    //debug("C handler\r\n");
}
 */
void CommandLayer::sendC() {
	LOG_DEBUG(LOG_CCI, "sendC");
	CciCsi::CResponse *resp = (CciCsi::CResponse*)response.getData();
	resp->type = CciCsi::Operation_C;
	resp->magic1 = '0';
	resp->magic2 = '0';
	resp->magic3 = '0';
	resp->magic4 = '0';
	resp->magic5 = '0';
	resp->magic6 = '0';
	resp->magic7 = '0';
	response.setLen(sizeof(CciCsi::CResponse));
	packetLayer->sendPacket(&response);
}

/*
void Franke::S_Handler()
{
    m_sendBuf[0] = CM_STX;
    m_sendBuf[1] = 'S';
    m_sendBuf[2] = 0x31;
    m_sendBuf[3] = 0x80;
    m_sendBuf[4] = 0x31;
    m_sendBuf[5] = 0x80;

    if (m_operator->isPackageRequest()) {
        m_sendBuf[3] |= 0x20;
    }

    m_parser->addCRC(m_sendBuf, 6);
    m_parser->sendData(m_sendBuf, 10);

    //debug("S handler\r\n");
}
 */
void CommandLayer::sendStatusNoAction() {
	StatusResponse *resp = (StatusResponse*)response.getData();
	resp->type = Operation_Status;
	resp->status = Status_NoAction;
	resp->IF_STAT = 0x80;
	resp->TO_PS.set(FRANKE_APPROVING_TIMEOUT);
	resp->reserved = 0x80;
	response.setLen(sizeof(StatusResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendStatusReady() {
	StatusResponse *resp = (StatusResponse*)response.getData();
	resp->type = Operation_Status;
	resp->status = Status_Ready;
	resp->IF_STAT = 0x80;
	resp->TO_PS.set(FRANKE_APPROVING_TIMEOUT);
	resp->reserved = 0x80;
	response.setLen(sizeof(StatusResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendInquiryApproved() {
	InquiryResponse *resp = (InquiryResponse*)response.getData();
	resp->type = Operation_Inquiry;
	resp->verdict = Verdict_Approve;
	response.setLen(sizeof(InquiryResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendInquiryDenied() {
	InquiryResponse *resp = (InquiryResponse*)response.getData();
	resp->type = Operation_Inquiry;
	resp->verdict = Verdict_Deny;
	response.setLen(sizeof(InquiryResponse));
	packetLayer->sendPacket(&response);
}

/*
void Franke::B_Handler()
{
    char str[4] = {0, 0, 0, 0};
    uint16_t plu;

    // Получаем PLU напитка
    memcpy (str, m_cciCommand.data, 3);
    plu = (uint16_t)strtol(str, NULL, 10);

    m_sendBuf[0] = CM_STX;
    m_sendBuf[1] = 'B';

    if (m_operator->isPluReleased(plu))
    {
        m_sendBuf[2] = '1';
        m_operator->pluIsPoured(plu);
    }
    else
        m_sendBuf[2] = '0';

    m_parser->addCRC(m_sendBuf, 3);
    m_parser->sendData(m_sendBuf, 7);
}
x42;x31;x30;x30;x30;x30;x30;x30;x30;x30;x30;x30;x30;
 */
void CommandLayer::sendBApproved() {
	CciCsi::BResponse *resp = (CciCsi::BResponse*)response.getData();
	resp->type = CciCsi::Operation_B;
	resp->magic1 = '1'; // '1' разрешено
	response.setLen(sizeof(CciCsi::BResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendBDenied() {
	CciCsi::BResponse *resp = (CciCsi::BResponse*)response.getData();
	resp->type = CciCsi::Operation_B;
	resp->magic1 = '0'; // '0' запрещено
	response.setLen(sizeof(CciCsi::BResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::procControl(uint8_t control) {
	LOG_DEBUG(LOG_CCI, "procControl " << control);

}

void CommandLayer::procError(Error ) {
	LOG_DEBUG(LOG_CCI, "procError");

}

}
}

