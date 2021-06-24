#ifndef _FACEIDORDERMASTER_H_
#define _FACEIDORDERMASTER_H_

#include "SaleManagerOrderCore.h"
#include "common/mqtt/include/MqttClient.h"

class FaceIdOrderMaster : public OrderMasterInterface, public EventSubscriber {
public:
    FaceIdOrderMaster(TcpIp *conn, TimerEngine *timerEngine, EventEngineInterface *eventengine,
    		const char* faceIdDevice, const char* addr, uint16_t port, const char* username, const char* password);
    virtual ~FaceIdOrderMaster();

    void setOrder(Order *order) override;
    void reset() override;
    void connect() override;
    void check() override;
	void checkPinCode(const char *pincode) override;
    void distribute(uint16_t cid) override;
    void complete() override;

    void proc(EventEnvelope *envelope) override;
    void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Init,
		State_Wait,
		State_Check,
		State_CheckPinCode,
		State_Vending,
	};

	EventEngineInterface *eventEngine = nullptr;
	TimerEngine *timerEngine = nullptr;
	Timer *timer = nullptr;
	EventDeviceId deviceId;
	Order *order = nullptr;
	Mqtt::Client *client = nullptr;
	const char* faceIdDevice; // Идентификатор устройства, с которым мы работаем
	const char *addr;
	uint16_t port;
	const char *userName;
	const char *passWord;
	State state;
	String payload;

	void stateInitEvent(EventEnvelope *envelope);
	void stateInitEventConnectComplete();

	void gotoStateWait();
	void stateWaitEvent(EventEnvelope *envelope);

	void gotoStateCheck();
	void stateCheckEvent(EventEnvelope *envelope);
	void stateCheckEventIncommingMessage();
	void procBeverageMessage(JsonNode *payload);
	void procPinCodeMessage(JsonNode *payload);
	void procErrorMessage();
	void stateCheckTimeout();

	void gotoStateCheckPinCode(const char *pincode);

	void stateVendingEvent(EventEnvelope *envelope);
};

#endif // _FACEIDORDERMASTER_H_
