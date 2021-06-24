/*
 * UsbController.cpp
 *
 *  Created on: 10 сент. 2019 г.
 *      Author: Administrator
 */

#include "config.h"
#ifdef DEVICE_USB

#include "defines.h"
#include "common/logger/include/Logger.h"

#include "UsbController.h"
#include "usbh_core.h"
#include "usbh_vendor_PAX_D200.h"

#define TIMER_VALUE 					5000

UsbController::UsbController(USB *owner, USBH_HandleTypeDef *phost, TimerEngine *timerEngine): owner(owner), phost(phost), timerEngine(timerEngine)
{
	state = Stop;
	timer = timerEngine->addTimer<UsbController, &UsbController::procTimer>(this);
}

UsbController::~UsbController()
{
}

void UsbController::userProcess(uint8_t id)
{
	switch (id)
	{
	case HOST_USER_CLASS_SELECTED:
	case HOST_USER_SELECT_CONFIGURATION:
	break;

	case HOST_USER_DISCONNECTION:
		LOG_DEBUG(LOG_USB_CONTROLLER, "Start control timer, reason: USB disconnection");
		timer->start(TIMER_VALUE);
	break;

	case HOST_USER_CLASS_ACTIVE:
		if (strcmp(Vendor_PAX_D200_Class.Name, phost->pActiveClass->Name) == 0)
		{
			// Если активный класс - PAX_D200 то делаем переинициализацию драйвера.
			LOG_DEBUG(LOG_USB_CONTROLLER, "Start control timer, reason: User class PAX_D200 is active");
			timer->start(TIMER_VALUE);
			state = Stop;
		}
		else
		{
			LOG_DEBUG(LOG_USB_CONTROLLER, "Stop control timer, reason: USB class not PAX_D200");
			timer->stop();
		}
	break;

	case HOST_USER_CONNECTION:
		// CDC класс сюда не попадает, он останавливается на HOST_USER_CLASS_ACTIVE !

		LOG_DEBUG(LOG_USB_CONTROLLER, "Stop control timer for PAX_D200, reason: USB Connected");
		timer->stop();
	break;

	default:
		LOG_ERROR(LOG_USB_CONTROLLER, "UsbController, unknown userProcess id: " << id);
	break;
	}
}

void UsbController::procTimer()
{
	LOG_DEBUG(LOG_USB_CONTROLLER, "UsbController PAX_D200 timeout, state: " << (int)state);

	switch (state)
	{
		case Stop:
			state = Start;
			timer->start(TIMER_VALUE);
			owner->stop();
		break;

		case Start:
			state = Stop;
			timer->start(TIMER_VALUE);
			owner->start();
		break;
	}
}


#endif
