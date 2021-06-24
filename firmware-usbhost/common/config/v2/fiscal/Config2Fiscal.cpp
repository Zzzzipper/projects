#include "Config2Fiscal.h"
#include "evadts/EvadtsProtocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CONFIG_FISCAL_VERSION 2

#pragma pack(push,1)
struct Config2FiscalStruct {
	uint8_t version;
	uint16_t kkt;
	uint16_t kktInterface;
	StrParam<DOMAIN_NAME> kktAddr;
	uint16_t kktPort;
	StrParam<DOMAIN_NAME> ofdAddr;
	uint16_t ofdPort;
	StrParam<INN_SIZE> inn;
	StrParam<AUTOMAT_NUMBER_SIZE> automatNumber;
	StrParam<POINT_NAME> pointName;
	StrParam<POINT_ADDR> pointAddr;
	uint8_t crc[1];
};
#pragma pack(pop)

Config2Fiscal::Config2Fiscal() :
	kktAddr(DOMAIN_NAME, DOMAIN_NAME),
	ofdAddr(DOMAIN_NAME, DOMAIN_NAME),
	inn(INN_SIZE, INN_SIZE),
	automatNumber(AUTOMAT_NUMBER_SIZE, AUTOMAT_NUMBER_SIZE),
	pointName(POINT_NAME, POINT_NAME),
	pointAddr(POINT_ADDR, POINT_ADDR)
{
	setDefault();
}

MemoryResult Config2Fiscal::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	setDefault();
	MemoryResult result = save();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Fiscal data init failed");
		return result;
	}
	result = authPublicKey.init(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Auth public key init failed");
		return result;
	}
	result = authPrivateKey.init(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Auth private key init failed");
		return result;
	}
	result = signPrivateKey.init(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Sing private key init failed");
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config2Fiscal::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();

	Config2FiscalStruct data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong fiscal data CRC");
		return result;
	}
	if(data.version != CONFIG_FISCAL_VERSION) {
		LOG_ERROR(LOG_CFG, "Unsupported fiscal data version " << data.version);
		return MemoryResult_WrongVersion;
	}
	setKkt(data.kkt);
	setKktInterface(data.kktInterface);
	setKktAddr(data.kktAddr.get());
	setKktPort(data.kktPort);
	if(data.kkt == Config2Fiscal::Kkt_OrangeDataEphor || data.kkt == Config2Fiscal::Kkt_OrangeData) {
		setOfdAddr("");
		setOfdPort(0);
		setGroup(data.ofdAddr.get());
	} else {
		setOfdAddr(data.ofdAddr.get());
		setOfdPort(data.ofdPort);
		setGroup("");
	}
	setINN(data.inn.get());
	setAutomatNumber(data.automatNumber.get());
	setPointName(data.pointName.get());
	setPointAddr(data.pointAddr.get());

	result = authPublicKey.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Auth public key load failed");
		return result;
	}
	result = authPrivateKey.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Auth private key load failed");
		return result;
	}
	result = signPrivateKey.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Sign private key load failed");
		return result;
	}

	LOG_INFO(LOG_CFG, "Fiscal config OK");
	return MemoryResult_Ok;
}

MemoryResult Config2Fiscal::save() {
	memory->setAddress(address);
	Config2FiscalStruct data;
	data.version = CONFIG_FISCAL_VERSION;
	data.kkt = kkt;
	data.kktInterface = kktInterface;
	data.kktAddr.set(kktAddr.getString());
	data.kktPort = kktPort;
	if(data.kkt == Config2Fiscal::Kkt_OrangeDataEphor || data.kkt == Config2Fiscal::Kkt_OrangeData) {
		data.ofdAddr.set(group.getString());
		data.ofdPort = 0;
	} else {
		data.ofdAddr.set(ofdAddr.getString());
		data.ofdPort = ofdPort;
	}
	data.inn.set(inn.getString());
	data.automatNumber.set(automatNumber.getString());
	data.pointName.set(pointName.getString());
	data.pointAddr.set(pointAddr.getString());

	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config2Fiscal::setDefault() {
	setKkt(Kkt_None);
	setKktInterface(KktInterface_Ethernet);
	setKktAddr("192.168.1.210");
	setKktPort(0);
	setOfdAddr("");
	setOfdPort(0);
	setINN("");
	setPointName("");
	setPointAddr("");
	setGroup("");
}

void Config2Fiscal::setKkt(uint16_t kkt) {
	this->kkt = kkt;
}

uint16_t Config2Fiscal::getKkt() const {
	return kkt;
}

void Config2Fiscal::setKktInterface(uint16_t kktInterface) {
	this->kktInterface = kktInterface;
}

uint16_t Config2Fiscal::getKktInterface() const {
	return kktInterface;
}

void Config2Fiscal::setKktAddr(const char *addr) {
	this->kktAddr = addr;
}

const char *Config2Fiscal::getKktAddr() const {
	return kktAddr.getString();
}

void Config2Fiscal::setKktPort(uint16_t port) {
	this->kktPort = port;
}

uint16_t Config2Fiscal::getKktPort() const {
	return kktPort;
}

void Config2Fiscal::setOfdAddr(const char *addr) {
	this->ofdAddr = addr;
}

const char *Config2Fiscal::getOfdAddr() const {
	return ofdAddr.getString();
}

void Config2Fiscal::setOfdPort(uint16_t port) {
	this->ofdPort = port;
}

uint16_t Config2Fiscal::getOfdPort() const {
	return ofdPort;
}

void Config2Fiscal::setAutomatNumber(const char *automatNumber) {
	this->automatNumber = automatNumber;
}

const char *Config2Fiscal::getAutomatNumber() const {
	return automatNumber.getString();
}

void Config2Fiscal::setINN(const char *inn) {
	this->inn = inn;
}

const char *Config2Fiscal::getINN() const {
	return inn.getString();
}

void Config2Fiscal::setPointName(const char *pointName) {
	this->pointName = pointName;
}

const char *Config2Fiscal::getPointName() const {
	return pointName.getString();
}

void Config2Fiscal::setPointAddr(const char *pointAddr) {
	this->pointAddr = pointAddr;
}

const char *Config2Fiscal::getPointAddr() const {
	return pointAddr.getString();
}

void Config2Fiscal::setGroup(const char *group) {
	this->group = group;
}

const char *Config2Fiscal::getGroup() const {
	return group.getString();
}

Config2Cert *Config2Fiscal::getAuthPublicKey() {
	return &authPublicKey;
}

bool Config2Fiscal::getAuthPublicKey(StringBuilder *buf) {
	return authPublicKey.load(buf);
}

bool Config2Fiscal::setAuthPublicKey(StringBuilder *data) {
	return authPublicKey.save(data);
}

Config2Cert *Config2Fiscal::getAuthPrivateKey() {
	return &authPrivateKey;
}

bool Config2Fiscal::getAuthPrivateKey(StringBuilder *buf) {
	return authPrivateKey.load(buf);
}

bool Config2Fiscal::setAuthPrivateKey(StringBuilder *data) {
	return authPrivateKey.save(data);
}

Config2Cert *Config2Fiscal::getSignPrivateKey() {
	return &signPrivateKey;
}

bool Config2Fiscal::getSignPrivateKey(StringBuilder *buf) {
	return signPrivateKey.load(buf);
}

bool Config2Fiscal::setSignPrivateKey(StringBuilder *data) {
	return signPrivateKey.save(data);
}
