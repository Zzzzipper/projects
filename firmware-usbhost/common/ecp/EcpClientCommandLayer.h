#ifndef COMMON_ECP_CLIENTCOMMANDLAYER_H
#define COMMON_ECP_CLIENTCOMMANDLAYER_H

#include "ecp/include/EcpClient.h"
#include "ecp/EcpClientPacketLayer.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "dex/include/DexDataGenerator.h"
#include "dex/include/DexDataParser.h"
#include "utils/include/Buffer.h"
#include "utils/include/Event.h"

namespace Ecp {

class ClientCommandLayer : public ClientPacketLayerInterface::Observer {
public:
	ClientCommandLayer(TimerEngine *timers, ClientPacketLayerInterface *packetLayer);
	virtual ~ClientCommandLayer();
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

	virtual void procConnect();
	virtual void procRecvData(const uint8_t *data, uint16_t dataLen);
	virtual void procRecvError(Error error);
	virtual void procDisconnect();

private:
	enum State {
		State_Idle = 0,
		State_Connecting,
		State_Setup,
		State_Ready,
		State_UploadStart,
		State_UploadData,
		State_UploadDataBusy,
		State_UploadEnd,
		State_UploadEndBusy,
		State_DownloadStart,
		State_DownloadData,
		State_Request,
		State_Disconnecting,
	};

	ClientPacketLayerInterface *packetLayer;
	TimerEngine *timers;
	EventCourier courier;
	Timer *timer;
	State state;
	Destination destination;
	Dex::DataGenerator *generator;
	Source source;
	Dex::DataParser *parser;
	Buffer request;
	Client::EventResponse eventResponse;

	void procError(uint8_t error);
	void procTimer();
	void gotoStateSetup();
	void stateSetupRecv(const uint8_t *data, uint16_t dataLen);

	void gotoStateUploadStart();
	void stateUploadStart(const uint8_t *data, uint16_t dataLen);
	void gotoStateUploadData();
	void stateUploadDataRecv(const uint8_t *data, uint16_t dataLen);
	void gotoStateUploadDataBusy();
	void stateUploadDataBusyTimeout();
	void gotoStateUploadEnd();
	void stateUploadEndRecv(const uint8_t *data, uint16_t dataLen);
	void gotoStateUploadEndBusy();
	void stateUploadEndBusyTimeout();

	void gotoStateDownloadStart();
	void stateDownloadStartRecv(const uint8_t *data, uint16_t dataLen);
	void gotoStateDownloadData();
	void stateDownloadDataRecv(const uint8_t *data, uint16_t dataLen);

	void stateRequestRecv(const uint8_t *data, uint16_t dataLen);
};

}

#endif
