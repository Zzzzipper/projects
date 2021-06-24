#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

namespace Mdb {

enum FiscalEventErrorParam {
	MdbEventErrorParam_Code = 0,
	MdbEventErrorParam_Data,
};

EventError::EventError(uint16_t type) :
	EventInterface(type)
{
}

EventError::EventError(EventDeviceId deviceId, uint16_t type) :
	EventInterface(deviceId, type),
	code(ConfigEvent::Type_None),
	data(MDB_ERROR_DATA_SIZE, MDB_ERROR_DATA_SIZE)
{
}

bool EventError::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint16(MdbEventErrorParam_Code, &code) == false) { return false; }
	if(envelope->getString(MdbEventErrorParam_Data, &data) == false) { return false; }
	return true;
}

bool EventError::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint16(MdbEventErrorParam_Code, code) == false) { return false; }
	if(envelope->addString(MdbEventErrorParam_Data, data.getString()) == false) { return false; }
	return true;
}

enum EventDepositeParam {
	EventDepositeParam_Route = 0,
	EventDepositeParam_Nominal,
};

EventDeposite::EventDeposite(EventDeviceId deviceId, uint16_t type, uint8_t route, uint32_t nominal) :
	EventInterface(deviceId, type),
	route(route),
	nominal(nominal)
{
}

void EventDeposite::set(uint16_t type, uint8_t route, uint32_t nominal) {
	this->type = type;
	this->route = route;
	this->nominal = nominal;
}

bool EventDeposite::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint8(EventDepositeParam_Route, &route) == false) { return false; }
	if(envelope->getUint32(EventDepositeParam_Nominal, &nominal) == false) { return false; }
	return true;
}

bool EventDeposite::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint8(EventDepositeParam_Route, route) == false) { return false; }
	if(envelope->addUint32(EventDepositeParam_Nominal, nominal) == false) { return false; }
	return true;
}

uint8_t calcCrc(const uint8_t *data, uint16_t len) {
	uint8_t chk = 0;
	for(uint16_t i = 0; i < len; i++) {
		chk += data[i];
	}
	return chk;
}

const char *deviceId2String(uint8_t deviceId) {
	switch(deviceId) {
	case Device_CoinChanger: return "CoinChanger";
	case Device_CashlessDevice1: return "CashlessDevice1";
	case Device_ComGateway: return "ComGateway";
	case Device_BillValidator: return "BillValidator";
	case Device_CoinHopper1: return "CoinHopper1";
	case Device_CashlessDevice2: return "CashlessDevice2";
	case Device_CoinHopper2: return "CoinHopper2";
	default: return "Unknown";
	}
}

DeviceContext::DeviceContext(uint32_t masterDecimalPoint, RealTimeInterface *realtime) :
	status(Status_NotFound),
	softwareVersion(0),
	converter(masterDecimalPoint),
	currency(RUSSIAN_CURRENCY_RUB),
	state(0),
	resetCount(0),
	protocolErrorCount(0),
	errors(realtime)
{
	this->manufacturer.set("   ");
	this->model.set("            ");
	this->serialNumber.set("            ");
}

void DeviceContext::setMasterDecimalPoint(uint32_t masterDecimalPoint) {
	this->converter.setMasterDecimalPoint(masterDecimalPoint);
}

uint32_t DeviceContext::getMasterDecimalPoint() const {
	return converter.getMasterDecimalPoint();
}

void DeviceContext::init(uint32_t deviceDecimalPoint, uint32_t scalingFactor) {
	this->converter.setDeviceDecimalPoint(deviceDecimalPoint);
	this->converter.setScalingFactor(scalingFactor);
}

uint16_t DeviceContext::getDecimalPoint() const {
	return converter.getDeviceDecimalPoint();
}

uint16_t DeviceContext::getScalingFactor() const {
	return converter.getScalingFactor();
}

uint32_t DeviceContext::value2money(uint32_t value) {
	return converter.convertDeviceToMaster(value);
}

uint32_t DeviceContext::money2value(uint32_t value) {
	return converter.convertMasterToDevice(value);
}

void DeviceContext::setStatus(Status status) {
	this->status = status;
}

DeviceContext::Status DeviceContext::getStatus() const {
	return status;
}

void DeviceContext::setManufacturer(const uint8_t *str, uint16_t strLen) {
	manufacturer.set((const char*)str, strLen);
}

const char *DeviceContext::getManufacturer() const {
	return manufacturer.get();
}

uint16_t DeviceContext::getManufacturerSize() const {
	return MDB_MANUFACTURER_SIZE;
}

void DeviceContext::setModel(const uint8_t *str, uint16_t strLen) {
	model.set((const char*)str, strLen);
}

const char *DeviceContext::getModel() const {
	return model.get();
}

uint16_t DeviceContext::getModelSize() const {
	return MDB_MODEL_SIZE;
}

void DeviceContext::setSerialNumber(const uint8_t *str, uint16_t strLen) {
	serialNumber.set((const char*)str, strLen);
}

const char *DeviceContext::getSerialNumber() const {
	return serialNumber.get();
}

uint16_t DeviceContext::getSerialNumberSize() const {
	return MDB_SERIAL_NUMBER_SIZE;
}

void DeviceContext::setSoftwareVersion(uint16_t softwareVersion) {
	this->softwareVersion = softwareVersion;
}

uint16_t DeviceContext::getSoftwareVersion() const {
	return softwareVersion;
}

void DeviceContext::setCurrency(uint16_t currency) {
	this->currency = currency;
}

uint16_t DeviceContext::getCurrency() const {
	return currency;
}

bool DeviceContext::addError(uint16_t code, const char *str) {
	status = DeviceContext::Status_Error;
	return errors.add(code, str);
}

void DeviceContext::removeError(uint16_t code) {
	errors.remove(code);
	if(errors.getSize() == 0) {
		status = DeviceContext::Status_Work;
	}
}

void DeviceContext::removeAll() {
	errors.removeAll();
	status = DeviceContext::Status_Work;
}

ConfigErrorList *DeviceContext::getErrors() {
	return &errors;
}

}
