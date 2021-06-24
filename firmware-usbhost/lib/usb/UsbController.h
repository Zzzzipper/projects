/*
 * UsbController.h
 *
 *  Created on: 10 сент. 2019 г.
 *      Author: Administrator
 */

#include "config.h"
#ifdef DEVICE_USB
#pragma once


#include "defines.h"
#include "common.h"

#include "usbh_def.h"
#include "common/timer/include/TimerEngine.h"
#include "USB.h"

class USB;

class UsbController
{
private:
	USB *owner;
	USBH_HandleTypeDef *phost;
	TimerEngine *timerEngine;
	Timer *timer;

	void procTimer();

	enum {
		Stop,
		Start,

	} state;

public:
	UsbController(USB *owner, USBH_HandleTypeDef *phost, TimerEngine *timerEngine);
	virtual ~UsbController();

	void userProcess(uint8_t id);
};

#endif
