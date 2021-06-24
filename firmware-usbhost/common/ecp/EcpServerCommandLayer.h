#ifndef COMMON_ECP_SERVERCOMMANDLAYER_H
#define COMMON_ECP_SERVERCOMMANDLAYER_H

#include "ecp/include/EcpServer.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "dex/include/DexDataParser.h"
#include "dex/include/DexDataGenerator.h"
#include "utils/include/Buffer.h"
#include "utils/include/Event.h"

namespace Ecp {

class ServerCommandLayer : public ServerPacketLayerInterface::Observer, public EventObserver {
public:
	ServerCommandLayer(TimerEngine *timers, ServerPacketLayerInterface *packetLayer);
	virtual ~ServerCommandLayer();
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

	virtual void procConnect();
	virtual void procRecvData(const uint8_t *data, uint16_t dataLen);
	virtual void procError(uint8_t error);
	virtual void procDisconnect();

	virtual void proc(Event *event);

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Ready,
		State_Recv,
		State_RecvAsync,
		State_RecvEnd,
		State_Send,
		State_Disconnecting,
	};

	ServerPacketLayerInterface *packetLayer;
	TimerEngine *timers;
	EventCourier courier;
	Timer *timer;
	State state;
	Dex::DataParser *modemFirmwareParser;
	Dex::DataParser *gsmFirmwareParser;
	Dex::DataParser *screenFirmwareParser;
	Dex::DataParser *configParser;
	Dex::DataParser *parser;
	Dex::DataGenerator *configGenerator;
	Dex::DataGenerator *generator;
	TableProcessor *processor;
	Dex::DataParser *configEraser;
	Buffer packet;

	void sendResponse(uint8_t command, Error errorCode);

	void stateReadyRecv(const uint8_t *data, uint16_t dataLen);
	void stateReadyCommandSetup();
	void stateReadyCommandUploadStart(const uint8_t *data, uint16_t dataLen);
	void stateReadyCommandDownloadStart(const uint8_t *data, uint16_t dataLen);
	void stateReadyCommandTableInfo(const uint8_t *data, uint16_t dataLen);
	void stateReadyCommandTableEntry(const uint8_t *data, uint16_t dataLen);
	void stateReadyCommandDateTime(const uint8_t *data, uint16_t dataLen);
	void stateReadyCommandConfigReset(const uint8_t *data, uint16_t dataLen);

	void stateRecvRecv(const uint8_t *data, uint16_t dataLen);
	void stateRecvCommandUploadData(const uint8_t *data, uint16_t dataLen);
	void stateRecvCommandUploadEnd();

	void gotoStateRecvAsync();
	void stateRecvAsyncEvent(Event *event);

	void gotoStateRecvEnd();
	void stateRecvEndEvent(Event *event);

	void stateSendRecv(const uint8_t *data, uint16_t dataLen);
	void stateSendCommandDownloadData(const uint8_t *data, uint16_t dataLen);
};

}

#endif
