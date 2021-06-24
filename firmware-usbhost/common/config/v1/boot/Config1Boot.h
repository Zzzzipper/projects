#ifndef COMMON_CONFIG1_BOOT_H_
#define COMMON_CONFIG1_BOOT_H_

#include "memory/include/Memory.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

class Config1Boot {
public:
	enum FirmwareState {
		FirmwareState_None			 = 0xF0,
		FirmwareState_UpdateRequired = 0xF1,
		FirmwareState_UpdateComplete = 0xF2,
	};

	enum FirmwareRelease {
		FirmwareRelease_None	 = 0x00,
		FirmwareRelease_Stable	 = 0x01,
		FirmwareRelease_Newest	 = 0x02,
		FirmwareRelease_Support1 = 0x81,
		FirmwareRelease_Support2 = 0x82,
		FirmwareRelease_Support3 = 0x83,
		FirmwareRelease_Support4 = 0x84,
		FirmwareRelease_Support5 = 0x85,
	};

	Config1Boot();
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();
	void setDefault();

	void setHardwareVersion(uint32_t version);
	uint32_t getHardwareVersion();
	void setFirmwareVersion(uint32_t version);
	uint32_t getFirmwareVersion();
	void setFirmwareState(FirmwareState value);
	uint8_t getFirmwareState();
	void setFirmwareRelease(uint8_t value);
	uint8_t getFirmwareRelease();

	void setServerDomain(const char *serverDomain);
	const char *getServerDomain();
	void setServerPort(uint16_t serverPort);
	uint16_t getServerPort();
	void setServerPassword(const char *serverPassword);
	const char *getServerPassword();

	void setGsmFirmwareVersion(const char *version);
	const char *getGsmFirmwareVersion() const;
	void setImei(const char *imei);
	const char *getImei() const;
	void setIccid(const char *iccid);
	const char *getIccid() const;
	void setOper(const char *oper);
	const char *getOper() const;
	void setPhone(const char *phone);
	const char *getPhone() const;
	void setGprsApn(const char *gprsApn);
	const char *getGprsApn() const;
	void setGprsUsername(const char *gprsUsername);
	const char *getGprsUsername() const;
	void setGprsPassword(const char *gprsPassword);
	const char *getGprsPassword() const;

private:
	Memory *memory;
	uint32_t address;
	uint32_t hardwareVersion;
	uint32_t firmwareVersion;
	uint8_t  firmwareState;
	uint8_t  firmwareRelease;
	StringBuilder serverDomain;
	uint16_t serverPort;
	StringBuilder serverPassword;
	StringBuilder gsmFirmwareVersion;
	StringBuilder imei;
	StringBuilder iccid;
	StringBuilder oper;
	StringBuilder phone;
	StringBuilder gprsApn;
	StringBuilder gprsUsername;
	StringBuilder gprsPassword;
};

#endif
