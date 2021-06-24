#ifndef COMMON_CONFIG_V3_AUTOMATSETTINGS_H_
#define COMMON_CONFIG_V3_AUTOMATSETTINGS_H_

#include "memory/include/Memory.h"
#include "cardreader/vendotek/Tlv.h"

class Config3AutomatSettings {
public:
	Config3AutomatSettings();
	void setDefault();
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	void setEthMac(uint8_t *mac);
	void setEthMac(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5);
	uint8_t *getEthMac();
	uint16_t getEthMacLen();
	void setEthAddr(uint32_t addr);
	uint32_t getEthAddr();
	void setEthMask(uint32_t addr);
	uint32_t getEthMask();
	void setEthGateway(uint32_t addr);
	uint32_t getEthGateway();

	void setFidType(uint16_t type);
	uint16_t getFidType();
	void setFidDevice(const char *device);
	const char *getFidDevice();
	void setFidAddr(const char *addr);
	const char *getFidAddr();
	void setFidPort(uint16_t port);
	uint16_t getFidPort();
	void setFidUsername(const char *username);
	const char *getFidUsername();
	void setFidPassword(const char *password);
	const char *getFidPassword();

private:
	Memory *memory;
	uint32_t address;

	uint8_t ethMac[6];
	uint32_t ethAddr;
	uint32_t ethMask;
	uint32_t ethGateway;

	uint16_t fidType;
	StringBuilder fidDevice;
	StringBuilder fidAddr;
	uint16_t fidPort;
	StringBuilder fidUsername;
	StringBuilder fidPassword;
};

#endif
