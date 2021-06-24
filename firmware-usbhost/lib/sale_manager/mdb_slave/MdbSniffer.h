#ifndef COMMON_MDB_SNIFFER_H
#define COMMON_MDB_SNIFFER_H

#include "mdb/slave/MdbSlave.h"
#include "mdb/master/MdbMasterReceiver.h"

class MdbSniffer : public MdbSlave, public MdbMasterReceiver::Customer {
public:
    MdbSniffer(Mdb::Device deviceType, EventEngineInterface *eventEngine) : MdbSlave(deviceType, eventEngine) {}
	virtual void reset() = 0;
	virtual bool isEnable() = 0;
};

#endif
