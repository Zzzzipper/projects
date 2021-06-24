#ifndef COMMON_CONFIG2_FISCAL_H_
#define COMMON_CONFIG2_FISCAL_H_

#include "Config2Cert.h"

#include "memory/include/Memory.h"
#include "utils/include/StringBuilder.h"

#define IPADDR_SIZE 15
#define DOMAIN_NAME 32
#define INN_SIZE 12
#define AUTOMAT_NUMBER_SIZE 20
#define POINT_NAME 64
#define POINT_ADDR 128

class Config2Fiscal {
public:
	enum Kkt {
		Kkt_None			 = 0,
		Kkt_Paykiosk01FA	 = 1,
		Kkt_KaznachejFA		 = 2,
		Kkt_RPSystem1FA		 = 3,
		Kkt_TerminalFA		 = 4,
		Kkt_OrangeData		 = 5,
		Kkt_ChekOnline		 = 6,
		Kkt_OrangeDataEphor	 = 7,
		Kkt_EphorOnline		 = 8,
		Kkt_Nanokassa		 = 9,
		Kkt_ServerOrangeData = 10,
		Kkt_ServerOrangeDataEphor = 11,
		Kkt_ServerOdf		 = 12,
		Kkt_ServerNanokassa	 = 13,
	};
	enum KktInterface {
		KktInterface_RS232   = 0,
		KktInterface_Ethernet= 1,
		KktInterface_GSM	 = 2,
	};

	Config2Fiscal();
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();
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

	void setINN(const char *addr);
	const char *getINN() const;
	void setAutomatNumber(const char *addr);
	const char *getAutomatNumber() const;
	void setPointName(const char *addr);
	const char *getPointName() const;
	void setPointAddr(const char *addr);
	const char *getPointAddr() const;
	void setGroup(const char *group);
	const char *getGroup() const;

	Config2Cert *getAuthPublicKey();
	bool getAuthPublicKey(StringBuilder *buf);
	bool setAuthPublicKey(StringBuilder *data);
	Config2Cert *getAuthPrivateKey();
	bool getAuthPrivateKey(StringBuilder *buf);
	bool setAuthPrivateKey(StringBuilder *data);
	Config2Cert *getSignPrivateKey();
	bool getSignPrivateKey(StringBuilder *buf);
	bool setSignPrivateKey(StringBuilder *data);

private:
	Memory *memory;
	uint32_t address;
	uint16_t kkt;
	uint16_t kktInterface;
	StringBuilder kktAddr;
	uint16_t kktPort;
	StringBuilder ofdAddr;
	uint16_t ofdPort;
	StringBuilder inn;
	StringBuilder automatNumber;
	StringBuilder pointName;
	StringBuilder pointAddr;
	StringBuilder group;
	Config2Cert authPublicKey;
	Config2Cert authPrivateKey;
	Config2Cert signPrivateKey;
};

#endif
