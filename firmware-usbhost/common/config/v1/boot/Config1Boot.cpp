#include "Config1Boot.h"
#include "evadts/EvadtsProtocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CONFIG_BOOT_VERSION 1
#define SERVER_DOMAIN_SIZE 32
#define SERVER_DOMAIN_DEFAULT "erp.ephor.online"
#define SERVER_PORT_DETAULT 443
#define SERVER_PASSWORD_SIZE 32
#define SERVER_PASSWORD_DEFAULT "12345678"
#define GSM_FIRMWARE_VERSION 32
#define IMEI_SIZE 20
#define ICCID_SIZE 20
#define OPER_SIZE 8 // AT+COPS?, format=1, 8 bytes
#define PHONE_SIZE 15
#define GPRS_APN_DEFAULT "internet"
#define GPRS_APN_SIZE 32 // todo: узнать максимальный размер
#define GPRS_USERNAME_DEFAULT "gdata"
#define GPRS_USERNAME_SIZE 32 // todo: узнать максимальный размер
#define GPRS_PASSWORD_DEFAULT "gdata"
#define GPRS_PASSWORD_SIZE 32 // todo: узнать максимальный размер

#pragma pack(push,1)
struct Config1BootStruct {
	uint8_t version;
	uint8_t firmwareState;
	uint8_t firmwareRelease;
	StrParam<SERVER_DOMAIN_SIZE> serverDomain;
	uint16_t serverPort;
	StrParam<SERVER_PASSWORD_SIZE> serverPassword;
	StrParam<IMEI_SIZE> imei;
	StrParam<PHONE_SIZE> phone;
	StrParam<GPRS_APN_SIZE> gprsApn;
	StrParam<GPRS_USERNAME_SIZE> gprsUsername;
	StrParam<GPRS_PASSWORD_SIZE> gprsPassword;
	uint8_t crc[1];
};
#pragma pack(pop)

Config1Boot::Config1Boot() :
	serverDomain(SERVER_DOMAIN_SIZE, SERVER_DOMAIN_SIZE),
	serverPassword(SERVER_PASSWORD_SIZE, SERVER_PASSWORD_SIZE),
	gsmFirmwareVersion(GSM_FIRMWARE_VERSION, GSM_FIRMWARE_VERSION),
	imei(IMEI_SIZE, IMEI_SIZE),
	iccid(ICCID_SIZE, ICCID_SIZE),
	oper(OPER_SIZE, OPER_SIZE),
	phone(PHONE_SIZE, PHONE_SIZE),
	gprsApn(GPRS_APN_SIZE, GPRS_APN_SIZE),
	gprsUsername(GPRS_USERNAME_SIZE, GPRS_USERNAME_SIZE),
	gprsPassword(GPRS_PASSWORD_SIZE, GPRS_PASSWORD_SIZE)
{
	setDefault();
}

MemoryResult Config1Boot::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	setDefault();
	return save();
}

MemoryResult Config1Boot::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	Config1BootStruct data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong boot config CRC");
		return result;
	}
	if(data.version != CONFIG_BOOT_VERSION) {
		LOG_ERROR(LOG_CFG, "Unsupported boot config version " << data.version);
		return MemoryResult_WrongVersion;
	}

	firmwareState = data.firmwareState;
	firmwareRelease = data.firmwareRelease;
	serverDomain = data.serverDomain.get();
	serverPort = data.serverPort;
	serverPassword = data.serverPassword.get();
	imei = data.imei.get();
	phone = data.phone.get();
	gprsApn = data.gprsApn.get();
	gprsUsername = data.gprsUsername.get();
	gprsPassword = data.gprsPassword.get();
	LOG_INFO(LOG_CFG, "Boot config OK");
	return MemoryResult_Ok;
}

MemoryResult Config1Boot::save() {
	memory->setAddress(address);
	Config1BootStruct data;
	data.version = CONFIG_BOOT_VERSION;
	data.firmwareState = firmwareState;
	data.firmwareRelease = firmwareRelease;
	data.serverDomain.set(serverDomain.getString());
	data.serverPort = serverPort;
	data.serverPassword.set(serverPassword.getString());
	data.imei.set(imei.getString());
	data.phone.set(phone.getString());
	data.gprsApn.set(gprsApn.getString());
	data.gprsUsername.set(gprsUsername.getString());
	data.gprsPassword.set(gprsPassword.getString());

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config1Boot::setDefault() {
	setHardwareVersion(0);
	setFirmwareVersion(0);
	setFirmwareState(FirmwareState_None);
	setFirmwareRelease(FirmwareRelease_Stable);
	setServerDomain(SERVER_DOMAIN_DEFAULT);
	setServerPort(SERVER_PORT_DETAULT);
	setServerPassword(SERVER_PASSWORD_DEFAULT);
	setGsmFirmwareVersion("");
	setImei("");
	setPhone("");
	setGprsApn(GPRS_APN_DEFAULT);
	setGprsUsername(GPRS_USERNAME_DEFAULT);
	setGprsPassword(GPRS_PASSWORD_DEFAULT);
}

void Config1Boot::setHardwareVersion(uint32_t version) {
	this->hardwareVersion = version;
}

uint32_t Config1Boot::getHardwareVersion() {
	return hardwareVersion;
}

void Config1Boot::setFirmwareVersion(uint32_t version) {
	this->firmwareVersion = version;
}

uint32_t Config1Boot::getFirmwareVersion() {
	return firmwareVersion;
}

void Config1Boot::setFirmwareState(FirmwareState value) {
	this->firmwareState = value;
}

uint8_t Config1Boot::getFirmwareState() {
	return firmwareState;
}

void Config1Boot::setFirmwareRelease(uint8_t value) {
	this->firmwareRelease = value;
}

uint8_t Config1Boot::getFirmwareRelease() {
	return firmwareRelease;
}

void Config1Boot::setServerDomain(const char *serverDomain) {
	this->serverDomain = serverDomain;
}

const char *Config1Boot::getServerDomain() {
	return serverDomain.getString();
}

void Config1Boot::setServerPort(uint16_t serverPort) {
	this->serverPort = serverPort;
}

uint16_t Config1Boot::getServerPort() {
	return serverPort;
}

void Config1Boot::setServerPassword(const char *serverPassword) {
	this->serverPassword = serverPassword;
}

const char *Config1Boot::getServerPassword() {
	return serverPassword.getString();
}

void Config1Boot::setGsmFirmwareVersion(const char *version) {
	this->gsmFirmwareVersion.set(version);
}

const char *Config1Boot::getGsmFirmwareVersion() const {
	return gsmFirmwareVersion.getString();
}

void Config1Boot::setImei(const char *imei) {
	this->imei = imei;
}

const char *Config1Boot::getImei() const {
	return imei.getString();
}

void Config1Boot::setIccid(const char *iccid) {
	this->iccid = iccid;
}

const char *Config1Boot::getIccid() const {
	return iccid.getString();
}

void Config1Boot::setOper(const char *oper) {
	this->oper = oper;
}

const char *Config1Boot::getOper() const {
	return oper.getString();
}

void Config1Boot::setPhone(const char *phone) {
	this->phone = phone;
}

const char *Config1Boot::getPhone() const {
	return phone.getString();
}

void Config1Boot::setGprsApn(const char *gprsApn) {
	this->gprsApn = gprsApn;
}

const char *Config1Boot::getGprsApn() const {
	return gprsApn.getString();
}

void Config1Boot::setGprsUsername(const char *gprsUsername) {
	this->gprsUsername = gprsUsername;
}

const char *Config1Boot::getGprsUsername() const {
	return gprsUsername.getString();
}

void Config1Boot::setGprsPassword(const char *gprsPassword) {
	this->gprsPassword = gprsPassword;
}

const char *Config1Boot::getGprsPassword() const {
	return gprsPassword.getString();
}
