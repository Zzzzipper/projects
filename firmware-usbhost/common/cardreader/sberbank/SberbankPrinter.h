#ifndef COMMON_SBERBANK_PRINTER_H
#define COMMON_SBERBANK_PRINTER_H

#include "SberbankProtocol.h"

#include "tcpip/include/TcpIp.h"

namespace Sberbank {

class Printer : public EventObserver {
public:
	Printer(PacketLayerInterface *packetLayer);
	void procRequest(MasterCallRequest *req);
	void proc(Event *event) override;

private:
	enum State {
		State_Idle = 0,
		State_Wait,
	};

	PacketLayerInterface *packetLayer;
	TcpIp *conn;
	State state;
	Buffer sendBuf;
	uint16_t recvMaxSize;

	void deviceLanOpen(MasterCallRequest *req);
	void deviceLanWrite(MasterCallRequest *req);
	void deviceLanClose(MasterCallRequest *req);
};

}

#endif
