#ifndef MDB_MASTER_TESTMDBMASTERSENDER_H_
#define MDB_MASTER_TESTMDBMASTERSENDER_H_

#include "mdb/master/MdbMaster.h"
#include "utils/include/TestEventObserver.h"

class MdbMasterTester {
public:
	MdbMasterTester(MdbMaster *master);

	void poll();
	bool recvData(const char *response);
	bool recvConfirm(const uint8_t control);
	void recvTimeout();

	uint8_t *getSendData() { return sender.getSendData(); }
	uint16_t getSendLen() { return sender.getSendLen(); }

private:
	class Sender : public MdbMasterSender {
	public:
		void sendConfirm(Mdb::Control control);
		void sendRequest();

		uint8_t *getSendData();
		uint16_t getSendLen();
		void clearSendData();
	};

	MdbMaster *master;
	Buffer recvBuffer;
	Sender sender;
};

#endif
