#ifndef COMMON_EXECUTIVE_SNIFFERPACKETLAYER_H
#define COMMON_EXECUTIVE_SNIFFERPACKETLAYER_H

#include "executive/ExeMasterReceiver.h"
#include "executive/ExeSlaveReceiver.h"

#include "uart/include/interface.h"
#include "utils/include/Buffer.h"
#include "config/include/StatStorage.h"

class ExeSnifferPacketLayer : public ExeSlaveReceiver::Customer, public ExeMasterReceiver::Customer {
public:
	class Customer {
	public:
		virtual ~Customer() {}
		virtual void recvByte(uint8_t command, uint8_t result) = 0;
		virtual void recvData(uint8_t *data, uint16_t dataLen) = 0;
	};

	ExeSnifferPacketLayer(Customer *customer, StatStorage *stat);
	virtual ~ExeSnifferPacketLayer();
	void init(AbstractUart *slaveUart, AbstractUart *masterUart);
	virtual void recvRequest(uint8_t req);
	virtual void recvResponse(uint8_t resp);

private:
	enum State {
		State_Byte,
		State_Data,
		State_DataSync,
	};

	State state;
	ExeSlaveReceiver slaveReceiver;
	ExeMasterReceiver masterReceiver;
	Customer *customer;
	StatNode *countCrcError;
	StatNode *countResponse;
	uint8_t req;
	Buffer data;

	void stateByteRecv(uint8_t req, uint8_t resp);
	void stateDataRecv(uint8_t req, uint8_t resp);
	void stateDataSyncRecv(uint8_t req, uint8_t resp);
	void printPair(uint8_t req, uint8_t resp);
};

#endif
