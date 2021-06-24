#ifndef COMMON_ECP_CLIENT_H
#define COMMON_ECP_CLIENT_H

#include "ecp/EcpProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "dex/include/DexDataGenerator.h"
#include "dex/include/DexDataParser.h"
#include "utils/include/Buffer.h"
#include "utils/include/Event.h"

namespace Ecp {

class ClientCommandLayer;
class ClientPacketLayer;

class Client {
public:
	enum EventType {
		Event_ConnectOK		= GlobalId_Ecp | 0x01,
		Event_ConnectError	= GlobalId_Ecp | 0x02, // uint16_t error
		Event_Disconnect	= GlobalId_Ecp | 0x03,
		Event_UploadOK		= GlobalId_Ecp | 0x04,
		Event_UploadError	= GlobalId_Ecp | 0x05, // uint16_t error
		Event_DownloadOK	= GlobalId_Ecp | 0x06,
		Event_DownloadError	= GlobalId_Ecp | 0x07, // uint16_t error
		Event_ResponseOK    = GlobalId_Ecp | 0x08, // EventResponse
		Event_ResponseError = GlobalId_Ecp | 0x09,
	};

	class EventResponse : public Event {
	public:
		EventResponse() : Event(Event_ResponseOK), data(ECP_PACKET_MAX_SIZE) {}
		Buffer data;
	};

	Client(TimerEngine *timers, AbstractUart *uart);
	virtual ~Client();
	void setObserver(EventObserver *observer);
	bool connect();
	void disconnect();
	bool uploadData(Destination destination, Dex::DataGenerator *generator);
	bool downloadData(Source source, Dex::DataParser *parser);
	bool getTableInfo(uint16_t tableId);
	bool getTableEntry(uint16_t tableId, uint32_t entryIndex);
	bool getDateTime();
	bool resetConfig();
	void cancel();

private:
	ClientCommandLayer *commandLayer;
	ClientPacketLayer *packetLayer;
};

}

#endif
