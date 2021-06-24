#include "Config1Fiscal.h"
#include "evadts/EvadtsProtocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CONFIG_FISCAL_VERSION 1
#define IPADDR_SIZE 15
#define DOMAIN_NAME 32

#pragma pack(push,1)
struct Config1FiscalStruct {
	uint8_t version;
	uint16_t kkt;
	uint16_t kktInterface;
	StrParam<IPADDR_SIZE> kktAddr;
	uint16_t kktPort;
	StrParam<IPADDR_SIZE> ofdAddr;
	uint16_t ofdPort;
	uint8_t crc[1];
};
#pragma pack(pop)

Config1Fiscal::Config1Fiscal() :
	kktAddr(DOMAIN_NAME, DOMAIN_NAME),
	ofdAddr(DOMAIN_NAME, DOMAIN_NAME)
{
	setDefault();
}

MemoryResult Config1Fiscal::load(Memory *memory) {
	Config1FiscalStruct data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong fiscal config CRC");
		return result;
	}
	if(data.version != CONFIG_FISCAL_VERSION) {
		LOG_ERROR(LOG_CFG, "Unsupported fiscal config version " << data.version);
		return MemoryResult_WrongVersion;
	}

	kkt = data.kkt;
	kktInterface = data.kktInterface;
	kktAddr = data.kktAddr.get();
	kktPort = data.kktPort;
	ofdAddr = data.ofdAddr.get();
	ofdPort = data.ofdPort;
	LOG_INFO(LOG_CFG, "Fiscal config OK");
	return MemoryResult_Ok;
}

MemoryResult Config1Fiscal::save(Memory *memory) {
	Config1FiscalStruct data;
	data.version = CONFIG_FISCAL_VERSION;
	data.kkt = kkt;
	data.kktInterface = kktInterface;
	data.kktAddr.set(kktAddr.getString());
	data.kktPort = kktPort;
	data.ofdAddr.set(ofdAddr.getString());
	data.ofdPort = ofdPort;

	MemoryCrc crc(memory);
	MemoryResult result = crc.writeDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

void Config1Fiscal::setDefault() {
	setKkt(Kkt_None);
	setKktInterface(KktInterface_Ethernet);
	setKktAddr("192.168.1.210");
	setKktPort(0);
	setOfdAddr("");
	setOfdPort(0);
}

void Config1Fiscal::setKkt(uint16_t kkt) {
	if(kkt > Kkt_MaxValue) {
		LOG_ERROR(LOG_CFG, "Wrong kkt value " << kkt);
		return;
	}
	this->kkt = kkt;
}

uint16_t Config1Fiscal::getKkt() const {
	return kkt;
}

void Config1Fiscal::setKktInterface(uint16_t kktInterface) {
	this->kktInterface = kktInterface;
}

uint16_t Config1Fiscal::getKktInterface() const {
	return kktInterface;
}

void Config1Fiscal::setKktAddr(const char *addr) {
	this->kktAddr = addr;
}

const char *Config1Fiscal::getKktAddr() const {
	return kktAddr.getString();
}

void Config1Fiscal::setKktPort(uint16_t port) {
	this->kktPort = port;
}

uint16_t Config1Fiscal::getKktPort() const {
	return kktPort;
}

void Config1Fiscal::setOfdAddr(const char *addr) {
	this->ofdAddr = addr;
}

const char *Config1Fiscal::getOfdAddr() const {
	return ofdAddr.getString();
}

void Config1Fiscal::setOfdPort(uint16_t port) {
	this->ofdPort = port;
}

uint16_t Config1Fiscal::getOfdPort() const {
	return ofdPort;
}
