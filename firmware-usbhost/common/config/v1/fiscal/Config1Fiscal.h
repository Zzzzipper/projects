#ifndef COMMON_CONFIG_V1_FISCAL_H_
#define COMMON_CONFIG_V1_FISCAL_H_

#include "memory/include/Memory.h"
#include "utils/include/StringBuilder.h"

class Config1Fiscal {
public:
	enum Kkt {
		Kkt_None         = 0,
		Kkt_Paykiosk01FA = 1,
		Kkt_KaznachejFA  = 2,
		Kkt_RPSystem1FA  = 3,
		Kkt_TerminalFA	 = 4,
		Kkt_OrangeData	 = 5,
		Kkt_MaxValue     = 5,
	};
	enum KktInterface {
		KktInterface_RS232    = 0,
		KktInterface_Ethernet = 1,
		KktInterface_GSM	  = 2,
	};

	Config1Fiscal();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);
	void setDefault();

	void setKkt(uint16_t kkt);
	uint16_t getKkt() const;
	void setKktInterface(uint16_t kktInterface);
	uint16_t getKktInterface() const;
	void setKktAddr(const char *addr);
	const char *getKktAddr() const;
	void setKktPort(uint16_t port);
	uint16_t getKktPort() const;
	void setOfdAddr(const char *addr);
	const char *getOfdAddr() const;
	void setOfdPort(uint16_t port);
	uint16_t getOfdPort() const;

private:
	uint16_t kkt;
	uint16_t kktInterface;
	StringBuilder kktAddr;
	uint16_t kktPort;
	StringBuilder ofdAddr;
	uint16_t ofdPort;
};

#endif
