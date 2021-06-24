#include "Config3AutomatSettings.h"

#include "memory/include/MemoryCrc.h"
#include "tcpip/include/TcpIpUtils.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

#include <string.h>

#define FID_DEVICE_SIZE 32
#define FID_ADDR_SIZE 32
#define FID_USERNAME_SIZE 32
#define FID_PASSWORD_SIZE 32

enum SettingId {
	SettingId_EthMac = 0,
	SettingId_EthAddr = 1,
	SettingId_EthMask = 2,
	SettingId_EthGateway = 3,
};

#pragma pack(push,1)
struct Config3AutomatSettingsData {
	uint8_t  ethMac[6];
	uint32_t ethAddr;
	uint32_t ethMask;
	uint32_t ethGateway;
	uint16_t fidType;
	StrParam<FID_DEVICE_SIZE> fidDevice;
	StrParam<FID_ADDR_SIZE> fidAddr;
	uint16_t fidPort;
	StrParam<FID_USERNAME_SIZE> fidUsername;
	StrParam<FID_PASSWORD_SIZE> fidPassword;
	uint8_t  crc[1];
};
#pragma pack(pop)

Config3AutomatSettings::Config3AutomatSettings() :
	fidType(0),
	fidDevice(FID_DEVICE_SIZE, FID_DEVICE_SIZE),
	fidAddr(FID_ADDR_SIZE, FID_ADDR_SIZE),
	fidPort(0),
	fidUsername(FID_USERNAME_SIZE, FID_USERNAME_SIZE),
	fidPassword(FID_PASSWORD_SIZE, FID_PASSWORD_SIZE)
{
}

void Config3AutomatSettings::setDefault() {
	setEthMac(0x20, 0x89, 0x84, 0x6A, 0x96, 0x01);
	setEthAddr(IPADDR4TO1(192,168,1,200));
	setEthMask(IPADDR4TO1(255,255,255,0));
	setEthGateway(IPADDR4TO1(192,168,1,1));
}

MemoryResult Config3AutomatSettings::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	setDefault();
	return save();
}

MemoryResult Config3AutomatSettings::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	Config3AutomatSettingsData data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong settings CRC");
		return result;
	}

	ethMac[0] = data.ethMac[0];
	ethMac[1] = data.ethMac[1];
	ethMac[2] = data.ethMac[2];
	ethMac[3] = data.ethMac[3];
	ethMac[4] = data.ethMac[4];
	ethMac[5] = data.ethMac[5];
	ethAddr = data.ethAddr;
	ethMask = data.ethMask;
	ethGateway = data.ethGateway;
	fidType = data.fidType;
	fidDevice.set(data.fidDevice.get());
	fidAddr.set(data.fidAddr.get());
	fidPort = data.fidPort;
	fidUsername.set(data.fidUsername.get());
	fidPassword.set(data.fidPassword.get());
	return MemoryResult_Ok;
}

MemoryResult Config3AutomatSettings::save() {
	memory->setAddress(address);
	Config3AutomatSettingsData data;
	data.ethMac[0] = ethMac[0];
	data.ethMac[1] = ethMac[1];
	data.ethMac[2] = ethMac[2];
	data.ethMac[3] = ethMac[3];
	data.ethMac[4] = ethMac[4];
	data.ethMac[5] = ethMac[5];
	data.ethAddr = ethAddr;
	data.ethMask = ethMask;
	data.ethGateway = ethGateway;
	data.fidType = fidType;
	data.fidDevice.set(fidDevice.getString());
	data.fidAddr.set(fidAddr.getString());
	data.fidPort = fidPort;
	data.fidUsername.set(fidUsername.getString());
	data.fidPassword.set(fidPassword.getString());

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config3AutomatSettings::setEthMac(uint8_t *mac) {
	ethMac[0] = mac[0];
	ethMac[1] = mac[1];
	ethMac[2] = mac[2];
	ethMac[3] = mac[3];
	ethMac[4] = mac[4];
	ethMac[5] = mac[5];
}

void Config3AutomatSettings::setEthMac(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5) {
	ethMac[0] = mac0;
	ethMac[1] = mac1;
	ethMac[2] = mac2;
	ethMac[3] = mac3;
	ethMac[4] = mac4;
	ethMac[5] = mac5;
}

uint8_t *Config3AutomatSettings::getEthMac() {
	return ethMac;
}

uint16_t Config3AutomatSettings::getEthMacLen() {
	return sizeof(ethMac);
}

void Config3AutomatSettings::setEthAddr(uint32_t addr) {
	this->ethAddr = addr;
}

uint32_t Config3AutomatSettings::getEthAddr() {
	return ethAddr;
}

void Config3AutomatSettings::setEthMask(uint32_t addr) {
	this->ethMask = addr;
}

uint32_t Config3AutomatSettings::getEthMask() {
	return ethMask;
}

void Config3AutomatSettings::setEthGateway(uint32_t addr) {
	this->ethGateway = addr;
}

uint32_t Config3AutomatSettings::getEthGateway() {
	return ethGateway;
}

void Config3AutomatSettings::setFidType(uint16_t type) {
	this->fidType = type;
}

uint16_t Config3AutomatSettings::getFidType() {
	return fidType;
}

void Config3AutomatSettings::setFidDevice(const char *device) {
	fidDevice.set(device);
}

const char *Config3AutomatSettings::getFidDevice() {
	return fidDevice.getString();
}

void Config3AutomatSettings::setFidAddr(const char *addr) {
	fidAddr.set(addr);
}

const char *Config3AutomatSettings::getFidAddr() {
	return fidAddr.getString();
}

void Config3AutomatSettings::setFidPort(uint16_t port) {
	this->fidPort = port;
}

uint16_t Config3AutomatSettings::getFidPort() {
	return fidPort;
}

void Config3AutomatSettings::setFidUsername(const char *username) {
	this->fidUsername.set(username);
}

const char *Config3AutomatSettings::getFidUsername() {
	return fidUsername.getString();
}

void Config3AutomatSettings::setFidPassword(const char *password) {
	this->fidPassword.set(password);
}

const char *Config3AutomatSettings::getFidPassword() {
	return fidPassword.getString();
}
