#ifndef LIB_SIM900_CERTIFICATE_H
#define LIB_SIM900_CERTIFICATE_H

#include "sim900/command/GsmCommand.h"
#include "tcpip/include/TcpIp.h"

#include <stdint.h>

class EventObserver;
class TimerEngine;
class Timer;

namespace Gsm {

class Certificate : public Command::Observer {
public:
	Certificate(TimerEngine *timers, CommandProcessor *commandProcessor);
	virtual ~Certificate();
	void setObserver(EventObserver *observer);
	void save();

private:
	enum State {
		State_Idle = 0,
		State_FsDrive,
		State_FsCreate,
		State_FsWrite,
		State_FsFileSize,
		State_FsRead,
		State_SslSetCert,
		State_SslSetCertWait,
	};
	TimerEngine *timers;
	CommandProcessor *commandProcessor;
	Timer *timer;
	EventCourier courier;
	State state;
	Command *command;
	StringBuilder path;
	uint8_t buf[4096];
	uint32_t filesize;

	void procTimer();

	void gotoStateFsDrive();
	void stateFsDriveResponse(Command::Result result, const char *data);
	void gotoStateFsCreate();
	void stateFsCreateResponse(Command::Result result);
	void gotoStateFsWrite();
	void stateFsWriteResponse(Command::Result result);
	void gotoStateFsFileSize();
	void stateFsFileSizeResponse(Command::Result result, const char *data);
	void gotoStateFsRead();
	void stateFsReadResponse(Command::Result result);
	void gotoStateSslSetCert();
	void stateSslSetCertResponse(Command::Result result);
	void gotoStateSslSetCertWait();
	void stateSslSetCertWaitEvent(const char *data);

public:
	virtual void proc(Event *event);
	virtual void procResponse(Command::Result result, const char *data);
	void procEvent(const char *data);
};

}

#endif
