#ifndef COMMON_INGENICO_DIALOGDIRECT_H
#define COMMON_INGENICO_DIALOGDIRECT_H

#include "IngenicoProtocol.h"

#include "tcpip/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "utils/include/Event.h"

namespace Ingenico {

class DialogDirect {
public:
	enum EventType {
		Event_Text	 = GlobalId_DialogDirect | 0x01,
		Event_Button = GlobalId_DialogDirect | 0x02,
		Event_Cancel = GlobalId_DialogDirect | 0x03,
		Event_Close	 = GlobalId_DialogDirect | 0x04,
	};

	DialogDirect(PacketLayerInterface *packetLayer, Packet *req, TimerEngine *timers, EventObserver *observer);
	bool isEnabled();
	void showText(const char *text);
	void waitButton();
	void close();

	void procDl(StringParser *parser);
	void procEndtr();
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Start,
		State_Wait,
		State_Text,
		State_TextTimeout,
		State_Button,
		State_Close,
		State_CloseDelay,
	};

	enum Command {
		Command_None = 0,
		Command_Text,
		Command_Button,
		Command_Close,
	};

	PacketLayerInterface *packetLayer;
	Packet *req;
	Timer *timer;
	EventObserver *observer;
	State state;
	Command command;
	const char *text;
	const char *line;

	bool procCommand();

	void gotoStateStart();
	void stateStartProcDl(StringParser *parser);

	void gotoStateText();
	void nextStateText();
	void stateTextProcDl(StringParser *parser);

	void gotoStateTextTimeout();
	void stateStateTextTimeoutTimeout();

	void gotoStateButton();
	void stateButtonProcDl(StringParser *parser);

	void gotoStateClose();
	void stateCloseProcEndtr();
	void stateCloseTimeout();

	void stateCloseDelayTimeout();
};

}

#endif
