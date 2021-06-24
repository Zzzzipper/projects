#ifndef MDB_MASTER_H_
#define MDB_MASTER_H_

#include "utils/include/Event.h"
#include "event/include/EventEngine.h"
#include "mdb/MdbProtocol.h"
#include "mdb/slave/MdbSlaveReceiver.h"
#include "mdb/slave/MdbSlaveSender.h"
#include "mdb/master/MdbMasterSender.h"

#include "platform/include/platform.h"

#include <stdint.h>
#include <stddef.h>

//+++
//todo: разбить на три класса: MdbDevice (getType,setObserver,reset), MdbSlave(setSlaveSender,setSlaveReceiver,recvRequest,recvRequestData), MdbMaster(setMasterSender,recvResponse)
class MdbMaster {
public:
	MdbMaster(Mdb::Device type, EventEngineInterface *eventEngine) : type(type), eventEngine(eventEngine) {}
	virtual ~MdbMaster() {}
	Mdb::Device getType() { return type; }
	virtual void initMaster(MdbMasterSender *sender) = 0;
	virtual void sendRequest() = 0;
	virtual void recvResponse(const uint8_t *data, uint16_t len, bool crc) = 0;
	virtual void timeoutResponse() = 0;

protected:
	void deliverEvent(EventInterface *event) { eventEngine->transmit(event); }

private:
	Mdb::Device type;
	EventEngineInterface *eventEngine;
};

#endif
