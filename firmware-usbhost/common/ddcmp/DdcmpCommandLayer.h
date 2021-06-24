#ifndef COMMON_DDCMP_COMMANDLAYER_H
#define COMMON_DDCMP_COMMANDLAYER_H

#include "DdcmpPacketLayer.h"
#include "DdcmpProtocol.h"

#include "timer/include/TimerEngine.h"
#include "dex/include/DexDataParser.h"
#include "dex/include/DexDataGenerator.h"

namespace Ddcmp {

class CommandLayer : public PacketLayerObserver {
public:
	CommandLayer(PacketLayerInterface *packetLayer, TimerEngine *timers);
	virtual ~CommandLayer();
	void recvAudit(Dex::DataParser *dataParser, Dex::CommandResult *commandResult);

private:
	enum State {
		State_Idle = 0,
		State_BaudRate,
		State_Auth,
		State_AuthResponse,
		State_AuditStart,
		State_AuditStartResponse,
		State_AuditRecv,
		State_Finish,
	};

	PacketLayerInterface *packetLayer;
	TimerEngine *timers;
	Timer *timer;
	State state;
	uint8_t receiveCnt;
	uint8_t sendCnt;
	Dex::DataParser *dataParser;
	Dex::CommandResult *commandResult;

	void gotoStateBaudRate();
	void stateBaudRatePacket(const uint8_t *data, uint16_t dataLen);

	void gotoStateAuth();
	void stateAuthControl(const uint8_t *data, uint16_t dataLen);
	void gotoStateAuthResponse();
	void stateAuthResponseData(const uint8_t *cmd, uint16_t cmdLen, const uint8_t *data, uint16_t dataLen);
	void stateAuthResponseAck();

	void gotoStateAuditStart();
	void stateAuditStartControl(const uint8_t *cmd, uint16_t cmdLen);
	void gotoStateAuditStartResponse();
	void stateAuditStartResponseData(const uint8_t *cmd, uint16_t cmdLen, const uint8_t *data, uint16_t dataLen);
	void gotoStateAuditRecv();
	void stateAuditRecvData(const uint8_t *cmd, uint16_t cmdLen, const uint8_t *data, uint16_t dataLen);
	void stateAuditRecvAck();

	void gotoStateFinish();
	void stateFinishControl(const uint8_t *cmd, uint16_t cmdLen);

public:
	void procTimer();
	virtual void recvControl(const uint8_t *data, const uint16_t len);
	virtual void recvData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen);
};

}

#endif
