#ifndef COMMON_ECP_SERVER_H
#define COMMON_ECP_SERVER_H

#include "ecp/EcpProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "dex/include/DexDataParser.h"
#include "dex/include/DexDataGenerator.h"
#include "utils/include/Buffer.h"
#include "utils/include/Event.h"

namespace Ecp {

class ServerCommandLayer;
class ServerPacketLayer;

class Server {
public:
	enum EventType {
		Event_Connect	 = GlobalId_Ecp | 0x01,
		Event_Disconnect = GlobalId_Ecp | 0x02,
	};

	Server(AbstractUart *uart, TimerEngine *timers);
	virtual ~Server();
	void setObserver(EventObserver *observer);
	void setFirmwareParser(Dex::DataParser *parser);
	void setGsmParser(Dex::DataParser *parser);
	void setScreenParser(Dex::DataParser *parser);
	void setConfigParser(Dex::DataParser *parser);
	void setConfigGenerator(Dex::DataGenerator *generator);
	void setConfigEraser(Dex::DataParser *eraser);
	void setTableProcessor(TableProcessor *processor);
	void reset();
	void shutdown();
	void disconnect();

private:
	ServerCommandLayer *commandLayer;
	ServerPacketLayer *packetLayer;
};

}

#endif
