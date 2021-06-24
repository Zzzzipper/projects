#ifndef MDB_SLAVE_TESTER_H_
#define MDB_SLAVE_TESTER_H_

#include "mdb/slave/MdbSlave.h"

class MdbSlaveTester {
public:
	MdbSlaveTester(MdbSlave *master);

	bool recvCommand(const char *command);
	bool recvConfirm(uint8_t control);

	uint8_t *getSendData() { return sender.getSendData(); }
	uint16_t getSendDataLen() { return sender.getSendDataLen(); }
	uint8_t *getSendConfirm() { return sender.getSendConfirm(); }
	uint16_t getSendConfirmLen() { return sender.getSendConfirmLen(); }
	void clearSendData() { sender.clearSendData(); }

private:
	class Sender : public MdbSlave::Sender {
	public:
		virtual void sendAnswer(Mdb::Control control) override;
		virtual void sendData() override;
		uint8_t *getSendData();
		uint16_t getSendDataLen();
		uint8_t *getSendConfirm();
		uint16_t getSendConfirmLen();
		void clearSendData();

	private:
		enum SendType {
			SendType_None = 0,
			SendType_Data,
			SendType_Confirm
		};
		uint8_t sendType;
	};

	class Receiver : public MdbSlave::Receiver {
	public:
		Receiver(MdbSlave *slave);
		virtual void recvAddress() override;
		virtual void recvSubcommand() override;
		virtual void recvRequest(uint16_t len) override;
		virtual void recvConfirm() override;
		bool recvCommand(const char *command);
		bool recvConfirm(uint8_t control);

	private:
		MdbSlave *slave;
		Buffer buf;
	};

	class Observer : public EventObserver {
	public:
		void proc(Event *event) override { type = event->getType(); }
		uint16_t getType() { return type; }
		void clear() { type = 0; }

	private:
		uint16_t type;
	};

	MdbSlave *slave;
	Sender sender;
	Receiver receiver;
};

#endif
