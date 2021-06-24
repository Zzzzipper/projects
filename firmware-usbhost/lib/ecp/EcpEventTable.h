#ifndef LIB_ECP_EVENTTABLE_H_
#define LIB_ECP_EVENTTABLE_H_

#include "common/config/include/ConfigModem.h"
#include "common/ecp/EcpProtocol.h"

class EcpEventTable : public Ecp::TableProcessor {
public:
	EcpEventTable(ConfigModem *config, RealTimeInterface *realtime);
	virtual ~EcpEventTable() {}
	virtual bool isTableExist(uint16_t tableId);
	virtual uint32_t getTableSize(uint16_t tableId);
	virtual uint16_t getTableEntry(uint16_t tableId, uint32_t entryIndex, uint8_t *buf, uint16_t bufSize);
	virtual uint16_t getDateTime(uint8_t *buf, uint16_t bufSize);

private:
	ConfigModem *config;
	RealTimeInterface *realtime;
};

#endif
