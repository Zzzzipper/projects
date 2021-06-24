#include <ccicsi/CciT3CommandLayer.h>
#include "CciCsiProtocol.h"

#include "mdb/slave/cashless/MdbSlaveCashless3.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Cci {
namespace T3 {

#define START_TIMEOUT 90000

ProductState::ProductState() {
	disable();
}

void ProductState::disable() {
	LOG_DEBUG(LOG_CCI, "disable");
	value = 0xFFFF;
}

void ProductState::enable() {
	LOG_DEBUG(LOG_CCI, "enable");
	value = 0;
}

void ProductState::disable(uint16_t cid) {
	value = value | (1 << (cid - 1));
}

void ProductState::enable(uint16_t cid) {
	value = value ^ (1 << (cid - 1));
}

void ProductState::set(Order *order) {
	value = 0;
	for(uint16_t i = 0; i < order->getLen(); i++) {
		OrderCell *cell = order->getByIndex(i);
		value = value | (1 << (cell->cid - 1));
	}
	value = ~value;
	LOG_DEBUG(LOG_CCI, "enableOrder " << order->getLen() << "/" << value);
}

uint32_t ProductState::get() {
	return value;
}

CommandLayer::CommandLayer(
	CciCsi::PacketLayerInterface *packetLayer,
	TimerEngine *timers,
	EventEngineInterface *eventEngine
) :
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
	disable();
}

CommandLayer::~CommandLayer() {
	timers->deleteTimer(this->timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::setOrder(Order *order) {
	this->order = order;
}

void CommandLayer::reset() {
	LOG_INFO(LOG_CCI, "reset");
	packetLayer->reset();
	gotoStateWait();
}

bool CommandLayer::isInited() {
	return false;
}

bool CommandLayer::isEnable() {
	return false;
}

void CommandLayer::disable() {
	productState.disable();
}

void CommandLayer::enable() {
	productState.enable();
}

void CommandLayer::approveVend() {
	LOG_INFO(LOG_CCI, "approveVend");
	productState.set(order);
	timer->start(START_TIMEOUT);
	this->state = State_Approve;
}

void CommandLayer::denyVend() {
	LOG_INFO(LOG_CCI, "denyVend");
	timer->stop();
	gotoStateWait();
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_CCI, "procTimer");
	switch(state) {
	case State_Approving: stateApprovingTimeout(); break;
	case State_Approve: stateApproveTimeout(); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited timeout " << state);
	}
}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "procPacket");
	LOG_DEBUG_HEX(LOG_CCI, data, dataLen);
	switch(state) {
	case State_Wait: stateWaitRequest(data, dataLen); break;
	case State_Approving: stateApprovingRequest(data, dataLen); break;
	case State_Approve: stateApproveRequest(data, dataLen); break;
	case State_Vending: stateVendingRequest(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited data " << state);
	}
}

void CommandLayer::gotoStateWait() {
	LOG_DEBUG(LOG_CCI, "gotoStateWait");
	enable();
	this->state = State_Wait;
}

void CommandLayer::stateWaitRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateWaitRequest");
	packetLayer->sendControl(CciCsi::Control_ACK);
	CciCsi::Header *hdr = (CciCsi::Header*)data;
	switch(hdr->type) {
	case CciCsi::Operation_BillingEnable: procBillingEnable(); break;
	case CciCsi::Operation_Buttons: sendButtons(); break;
	case CciCsi::Operation_Inquiry: stateWaitRequestInquiry(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

/*
void Thermoplan::T_Handler()
{
	// Обновляем состояние кнопок на экране КМ
	refreshButtonsState();

	m_operator->recvEventPackage(m_cciCommand.data, m_cciCommand.len);

	m_sendBuf[0] = CM_STX;
	m_sendBuf[1] = 'T';
	m_sendBuf[2] = '9';
	m_sendBuf[3] = m_parser->intToChar((m_buttonsState >> 12) & 0xF);
	m_sendBuf[4] = m_parser->intToChar((m_buttonsState >> 8) & 0xF);
	m_sendBuf[5] = m_parser->intToChar((m_buttonsState >> 4) & 0xF);
	m_sendBuf[6] = m_parser->intToChar(m_buttonsState & 0xF);
	m_sendBuf[7] = '0';
	m_sendBuf[8] = '0';
	m_sendBuf[9] = '0';

	if (m_operator->isPackageRequest())
		m_sendBuf[10] = m_parser->intToChar(m_operator->getPackageNumber());
	else
		m_sendBuf[10] = '0';

	m_parser->addCRC(m_sendBuf, 11);
	m_parser->sendData(m_sendBuf, 15);
}

char ParserCCI::intToChar(uint8_t val)
{
	if (val < 10)
		return val + 0x30;
	else
		return val + 0x37;
}

void Thermoplan::refreshButtonsState()
{
	// Если все доступны, то не делаем лишних вычислений
	if (m_operator->isAllBevsAvailable()) {
		m_buttonsState = 0;
		return;
	}

	if (m_operator->isAllBevsNotAvailable()) {
		m_buttonsState = 0xffff;
		return;
	}

	m_buttonsState = 0xffff; // Все напитки заблокированы

	for (uint8_t i = 0; i < 16; i++)
	{
		if (m_operator->isPositionAvailable(i + 1))
			m_buttonsState ^= 1 << i;
	}
}

bool Operator::isPositionAvailable(uint8_t position)
{
	Product product;
	int productIndex = getProductIndexFromPosition(position);

	if (productIndex == -1)
		return false;

	product.plu = m_bevs[productIndex].plu;
	product.group = m_bevs[productIndex].group;
	product.price = m_bevs[productIndex].price;

	switch (m_mode)
	{
	case opmode_Service :
		return true;
	break;

	case opmode_Ticket :
		return m_controller->isProductAvailableTicket(&product);
	break;

	case opmode_Payment :
		return m_controller->isProductAvailablePayment(&product);
	break;

	case opmode_Mixed :
		return m_controller->isProductAvailableMixed(&product);
	break;
	}

	return false;
}
 */
void CommandLayer::stateWaitRequestInquiry(const uint8_t *data, uint16_t ) {
	LOG_DEBUG(LOG_CCI, "stateWaitRequestInquiry");
	CciCsi::InquiryRequest *req = (CciCsi::InquiryRequest*)data;
	sendVendDenied();
	gotoStateApproving();
	EventUint16Interface event(deviceId, OrderDeviceInterface::Event_VendRequest, req->articleNumber.get());
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateApproving() {
	LOG_DEBUG(LOG_CCI, "gotoStateApproving");
	productState.disable();
	progressCount = 0;
	timer->start(1000);
	state = State_Approving;
}

void CommandLayer::stateApprovingRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateApprovingRequest");
	packetLayer->sendControl(CciCsi::Control_ACK);
	CciCsi::Header *hdr = (CciCsi::Header*)data;
	switch(hdr->type) {
	case CciCsi::Operation_BillingEnable: procBillingEnable(); break;
	case CciCsi::Operation_Buttons: stateApprovingRequestButtons(); break;
	case CciCsi::Operation_Inquiry: sendVendDenied(); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateApprovingRequestButtons() {
	LOG_DEBUG(LOG_CCI, "stateApprovingTimeout");
	if(progressCount > 0) {
		productState.disable(progressCount);
		progressCount++;
		if(progressCount > 4) { progressCount = 1; }
		productState.enable(progressCount);
	}
	sendButtons();
}

void CommandLayer::stateApprovingTimeout() {
	LOG_DEBUG(LOG_CCI, "stateApprovingTimeout");
	progressCount = 4;
}

void CommandLayer::stateApproveRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateVendApproveRequest");
	packetLayer->sendControl(CciCsi::Control_ACK);
	CciCsi::Header *hdr = (CciCsi::Header*)data;
	switch(hdr->type) {
	case CciCsi::Operation_BillingEnable: procBillingEnable(); break;
	case CciCsi::Operation_Buttons: sendButtons(); break;
	case CciCsi::Operation_Inquiry: stateApproveRequestInquiry(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateApproveRequestInquiry(const uint8_t *data, uint16_t ) {
	CciCsi::InquiryRequest *req = (CciCsi::InquiryRequest*)data;
	LOG_DEBUG(LOG_CCI, "stateApproveRequestInquiry " << req->articleNumber.get());
	timer->stop();
	if(order->hasId(req->articleNumber.get()) == true) {
		sendVendApproved();
		gotoStateWait();
		EventUint16Interface event(deviceId, OrderDeviceInterface::Event_VendCompleted, req->articleNumber.get());
		eventEngine->transmit(&event);
		return;
	} else {
		sendVendDenied();
		gotoStateWait();
		EventInterface event(deviceId, OrderDeviceInterface::Event_VendCancelled);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApproveTimeout() {
	LOG_DEBUG(LOG_CCI, "stateApproveTimeout");
	gotoStateWait();
	EventInterface event(deviceId, OrderDeviceInterface::Event_VendCancelled);
	eventEngine->transmit(&event);
}

void CommandLayer::stateVendingRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateVendingRequest");
	packetLayer->sendControl(CciCsi::Control_ACK);
	CciCsi::Header *hdr = (CciCsi::Header*)data;
	switch(hdr->type) {
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::procBillingEnable() {
	LOG_DEBUG(LOG_CCI, "procBillingEnable");
}

void CommandLayer::sendButtons() {
	LOG_DEBUG(LOG_CCI, "sendButtons " << productState.get());
	CciCsi::ButtonsResponse *resp = (CciCsi::ButtonsResponse*)response.getData();
	resp->type = CciCsi::Operation_Buttons;
	resp->magic1 = '9';
	resp->state.set(productState.get());
	resp->magic2[0] = '0';
	resp->magic2[1] = '0';
	resp->magic2[2] = '0';
	resp->magic3 = '0';
	response.setLen(sizeof(CciCsi::ButtonsResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendVendApproved() {
	LOG_DEBUG(LOG_CCI, "sendVendApproved");
	CciCsi::InquiryResponse *resp = (CciCsi::InquiryResponse*)response.getData();
	resp->type = CciCsi::Operation_Inquiry;
	resp->verdict = CciCsi::Verdict_Approve;
	response.setLen(sizeof(CciCsi::InquiryResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendVendDenied() {
	LOG_DEBUG(LOG_CCI, "sendVendDenied");
	CciCsi::InquiryResponse *resp = (CciCsi::InquiryResponse*)response.getData();
	resp->type = CciCsi::Operation_Inquiry;
	resp->verdict = CciCsi::Verdict_Deny;
	response.setLen(sizeof(CciCsi::InquiryResponse));
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
