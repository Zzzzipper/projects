#include "IngenicoDialogDirect.h"

#include "logger/include/Logger.h"

namespace Ingenico {

#define DIALOGDIRECT_TEXT_TIMEOUT 5000
#define DIALOGDIRECT_SEND_TIMEOUT 3000

DialogDirect::DialogDirect(
	PacketLayerInterface *packetLayer,
	Packet *req,
	TimerEngine *timers,
	EventObserver *observer
) :
	packetLayer(packetLayer),
	req(req),
	observer(observer),
	state(State_Idle)
{
	this->timer = timers->addTimer<DialogDirect, &DialogDirect::procTimer>(this);
}

bool DialogDirect::isEnabled() {
	return (state != State_Idle);
}

void DialogDirect::showText(const char *text) {
	LOG_DEBUG(LOG_ECL, "showText");
	this->text = text;
	if(state == State_Idle) {
		this->command = Command_Text;
		gotoStateStart();
		return;
	} else if(state == State_Wait || state == State_TextTimeout) {
		timer->stop();
		gotoStateText();
		return;
	} else {
		this->command = Command_Text;
		return;
	}
}

void DialogDirect::waitButton() {
	LOG_DEBUG(LOG_ECL, "waitButton");
	if(state == State_Idle) {
		this->command = Command_Button;
		gotoStateStart();
		return;
	} else if(state == State_Wait || state == State_TextTimeout) {
		timer->stop();
		gotoStateButton();
		return;
	} else {
		this->command = Command_Button;
		return;
	}
}

void DialogDirect::close() {
	LOG_DEBUG(LOG_ECL, "close");
	if(state == State_Idle) {
		timer->start(1);
		state = State_Close;
		return;
	} else if(state == State_Wait || state == State_TextTimeout) {
		timer->stop();
		gotoStateClose();
		return;
	} else {
		this->command = Command_Close;
		return;
	}
}

void DialogDirect::procDl(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procDl " << parser->unparsed());
	switch(state) {
	case State_Start: stateStartProcDl(parser); return;
	case State_Text: stateTextProcDl(parser); return;
	case State_Button: stateButtonProcDl(parser); return;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << state);
	}
}

void DialogDirect::procEndtr() {
	LOG_DEBUG(LOG_ECL, "procEndtr");
	if(command == Command_Close) {
		command = Command_None;
		stateCloseProcEndtr();
		return;
	}

	switch(state) {
	case State_Start: gotoStateStart(); return;
	case State_Wait: state = State_Idle; return;
	case State_Text: gotoStateStart(); return;
	case State_Button: gotoStateStart(); return;
	case State_Close: stateCloseProcEndtr(); return;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << state);
	}
}

void DialogDirect::procTimer() {
	switch(state) {
	case State_Start: gotoStateStart(); return;
	case State_TextTimeout: stateStateTextTimeoutTimeout(); return;
	case State_CloseDelay: stateCloseDelayTimeout(); return;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << state);
	}
}

bool DialogDirect::procCommand() {
	if(command == Command_Text) {
		command = Command_None;
		gotoStateText();
		return true;
	} else if(command == Command_Button) {
		command = Command_None;
		gotoStateButton();
		return true;
	} else if(command == Command_Close) {
		command = Command_None;
		gotoStateClose();
		return true;
	}
	return false;
}

void DialogDirect::gotoStateStart() {
	LOG_DEBUG(LOG_ECL, "gotoStateStart");
	req->clear();
	req->addNumber(2);
	req->addControl(Control_ESC);
	req->addNumber(32);
	req->addControl(Control_ESC);
	req->addControl(Control_ESC);
	packetLayer->sendPacket(req->getBuf());
	timer->start(DIALOGDIRECT_SEND_TIMEOUT);
	state = State_Start;
}

void DialogDirect::stateStartProcDl(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "stateStartProcDl");
	if(strcmp(parser->unparsed(), "START") == 0) {
		if(procCommand() == false) {
			LOG_ERROR(LOG_ECL, "Unsupported command " << command);
			return;
		}
	} else {
		gotoStateClose(); //?????
		return;
	}
}

//DL: WRLINE:DEST,ROWNUM,CELLNUM,LINE
void DialogDirect::gotoStateText() {
	LOG_DEBUG(LOG_ECL, "gotoStateText");
	line = text;
	nextStateText();
}

void DialogDirect::nextStateText() {
	LOG_DEBUG(LOG_ECL, "nextStateText");
	if(line[0] == '\0') {
		LOG_DEBUG(LOG_ECL, "Text complete");
		gotoStateTextTimeout();
		Event event(Event_Text);
		observer->proc(&event);
		return;
	}

	uint16_t i = 0;
	for(; line[i] != '\n' && line[i] != '\0'; i++) {}

	req->clear();
	req->addString("DL:WRLINE:0,");
	req->addString(line, i);
	packetLayer->sendPacket(req->getBuf());

	line = line + i;
	if(line[0] == '\n') { line++; }
	state = State_Text;
}

void DialogDirect::stateTextProcDl(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "stateTextProcDl");
#if 1
	if(strcmp(parser->unparsed(), "OK") == 0) {
		nextStateText();
		return;
	} else {
		gotoStateClose(); //?????
		return;
	}
#else
	LOG_DEBUG(LOG_ECL, "Text complete");
	gotoStateTextTimeout();
	Event event(Event_Text);
	observer->proc(&event);
	return;
#endif
}

void DialogDirect::gotoStateTextTimeout() {
	LOG_DEBUG(LOG_ECL, "gotoStateTextTimeout");
	if(command == Command_Button) {
		command = Command_None;
		gotoStateButton();
		return;
	} else if(command == Command_Close) {
		command = Command_None;
		gotoStateClose();
		return;
	}

	timer->start(DIALOGDIRECT_TEXT_TIMEOUT);
	state = State_TextTimeout;
}

void DialogDirect::stateStateTextTimeoutTimeout() {
	LOG_DEBUG(LOG_ECL, "stateStateTextTimeoutTimeout");
	gotoStateText();
}

void DialogDirect::gotoStateButton() {
	LOG_DEBUG(LOG_ECL, "gotoStateButton");
	if(command == Command_Text) {
		command = Command_None;
		gotoStateText();
		return;
	} else if(command == Command_Close) {
		command = Command_None;
		gotoStateClose();
		return;
	}

	req->clear();
	req->addString("DL:RDKEY:0,1");
//	req->addString("DL:RDLINE:0,1,3,0,5,4");
	packetLayer->sendPacket(req->getBuf());
	state = State_Button;
}

void DialogDirect::stateButtonProcDl(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "stateButtonProcDl " << parser->unparsed());
	if(strcmp(parser->unparsed(), "ERROR") == 0 || strcmp(parser->unparsed(), "TIMEOUT") == 0) {
		gotoStateButton();
		return;
	} else if(strcmp(parser->unparsed(), "CANCEL") == 0) {
		state = State_Wait;
		Event event(Event_Cancel);
		observer->proc(&event);
		return;
	} else {
		state = State_Wait;
		Event event(Event_Button);
		observer->proc(&event);
		return;
	}
}

void DialogDirect::gotoStateClose() {
	LOG_DEBUG(LOG_ECL, "gotoStateClose");
#if 1
	req->clear();
	req->addString("DL:END:");
	packetLayer->sendPacket(req->getBuf());
#else
	req->clear();
	req->addNumber(2);
	req->addControl(Control_ESC);
	req->addNumber(32);
	req->addControl(Control_ESC);
	req->addControl(Control_ESC);
	packetLayer->sendPacket(req->getBuf());
#endif
	state = State_Close;
}

void DialogDirect::stateCloseProcEndtr() {
	LOG_DEBUG(LOG_ECL, "stateCloseProcEndtr");
#if 0
	state = State_Idle;
	Event event(Event_Close);
	observer->proc(&event);
#else
	req->clear();
	req->addString("OK");
	packetLayer->sendPacket(req->getBuf());
	timer->start(100);
	state = State_CloseDelay;
#endif
}

void DialogDirect::stateCloseTimeout() {
	LOG_DEBUG(LOG_ECL, "stateCloseTimeout");
	state = State_Idle;
	Event event(Event_Close);
	observer->proc(&event);
}


void DialogDirect::stateCloseDelayTimeout() {
	LOG_DEBUG(LOG_ECL, "stateCloseDelayTimeout");
	state = State_Idle;
	Event event(Event_Close);
	observer->proc(&event);
}

}
