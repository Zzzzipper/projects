#include "LoyalityScreen.h"

#include "logger/include/Logger.h"

#define SCREEN_REDRAW_TIMEOUT 2000
#define SCREEN_PROGRESS_TIMEOUT 750
#define SCREEN_TEMP_TIMEOUT 20000

LoyalityScreen::LoyalityScreen(
	Screen *screen,
	TimerEngine *timerEngine
) :
	screen(screen),
	timerEngine(timerEngine),
	state(State_Idle),
	permText(128, 128),
	tempHeader(16, 16),
	tempText(128, 128)
{
	redrawTimer = timerEngine->addTimer<LoyalityScreen, &LoyalityScreen::procRedrawTimer>(this);
}

void LoyalityScreen::reset() {
	LOG_DEBUG(LOG_SCREEN, "reset");
	redrawTimer->start(1000);
	clear();
}

bool LoyalityScreen::drawQrCode(const char *header, const char *footer, const char *text) {
	LOG_DEBUG(LOG_SCREEN, "drawQrCode");
	tempCommand = Command_drawQrCode;
	tempHeader.set(header);
	tempText.set(text);

	if(procDrawTempQrCode() == false) {
		gotoStateDrawTemp();
		return true;
	} else {
		gotoStateWaitTemp();
		return true;
	}
}

bool LoyalityScreen::drawText(const char *text, uint8_t fontMultipleSize, uint16_t textClr, uint16_t backgroundClr) {
	LOG_DEBUG(LOG_SCREEN, "drawText");
	permCommand = Command_drawText;
	permText.set(text);
	permFontSize = fontMultipleSize;
	permTextClr = textClr;
	permBackgroundClr = backgroundClr;

	if(procDrawPermText() == false) {
		gotoStateDrawPerm();
		return true;
	} else {
		gotoStateIdle();
		return true;
	}
}

bool LoyalityScreen::drawProgress(const char *text, uint8_t fontMultipleSize, uint16_t textClr, uint16_t backgroundClr) {
	LOG_DEBUG(LOG_SCREEN, "drawProgress");
	permCommand = Command_drawProgress;
	permText.set(text);
	permText.addStr("\n\n");
	permFontSize = fontMultipleSize;
	permTextClr = textClr;
	permBackgroundClr = backgroundClr;

	if(procDrawPermProgress() == false) {
		gotoStateDrawPerm();
		return true;
	} else {
		gotoStateWaitProgress();
		return true;
	}
}

bool LoyalityScreen::drawImage() {
	LOG_DEBUG(LOG_SCREEN, "drawImage");
	permCommand = Command_drawImage;

	if(procDrawPermImage() == false) {
		gotoStateDrawPerm();
		return true;
	} else {
		gotoStateIdle();
		return true;
	}
}

bool LoyalityScreen::clear() {
	LOG_DEBUG(LOG_SCREEN, "clear");
	permCommand = Command_clear;

	if(screen->clearDisplay() == false) {
		gotoStateDrawPerm();
		return true;
	} else {
		gotoStateIdle();
		return true;
	}
}

void LoyalityScreen::procRedrawTimer() {
	LOG_DEBUG(LOG_SCREEN, "procRedrawTimer");
	switch(state) {
	case State_DrawPerm: stateDrawPermTimeout(); return;
	case State_WaitProgress: stateWaitProgressTimeout(); return;
	case State_DrawTemp: stateDrawTempTimeout(); return;
	case State_WaitTemp: stateWaitTempTimeout(); return;
	default: LOG_ERROR(LOG_SCREEN, "Unwaited timeout " << state);
	}
}

void LoyalityScreen::gotoStateIdle() {
	LOG_DEBUG(LOG_SCREEN, "gotoStateIdle");
	redrawTimer->stop();
	state = State_Idle;
}

void LoyalityScreen::gotoStateDrawPerm() {
	LOG_DEBUG(LOG_SCREEN, "gotoStateDrawPerm");
	redrawTimer->start(SCREEN_REDRAW_TIMEOUT);
	state = State_DrawPerm;
}

void LoyalityScreen::stateDrawPermTimeout() {
	LOG_DEBUG(LOG_SCREEN, "stateDrawPermTimeout");
	redrawTimer->start(SCREEN_REDRAW_TIMEOUT);
	switch(permCommand) {
		case Command_drawText: {
			if(procDrawPermText() == true) {
				gotoStateIdle();
				return;
			}
			break;
		}
		case Command_drawProgress: {
			if(procDrawPermProgress() == true) {
				gotoStateWaitProgress();
				return;
			}
			break;
		}
		case Command_drawImage: {
			if(procDrawPermImage() == true) {
				gotoStateIdle();
				return;
			}
			break;
		}
		case Command_clear: {
			if(procPermClear() == true) {
				gotoStateIdle();
				return;
			}
			break;
		}
		default: {
			gotoStateIdle();
			return;
		}
	}
}

void LoyalityScreen::gotoStateWaitProgress() {
	LOG_DEBUG(LOG_SCREEN, "gotoStateWaitProgress");
	lastCount = 0;
	redrawTimer->start(SCREEN_PROGRESS_TIMEOUT);
	state = State_WaitProgress;
}

void LoyalityScreen::stateWaitProgressTimeout() {
	LOG_DEBUG(LOG_SCREEN, "stateWaitProgressTimeout");
	redrawTimer->start(1000);
	lastCount++;
	if(lastCount == 1) {
		screen->drawTextOnCenter("\n\n .....", permFontSize, permTextClr, permBackgroundClr);
	} else if(lastCount == 2) {
		screen->drawTextOnCenter("\n\n. ....", permFontSize, permTextClr, permBackgroundClr);
	} else if(lastCount == 3) {
		screen->drawTextOnCenter("\n\n.. ...", permFontSize, permTextClr, permBackgroundClr);
	} else if(lastCount == 4) {
		screen->drawTextOnCenter("\n\n... ..", permFontSize, permTextClr, permBackgroundClr);
	} else if(lastCount == 5) {
		screen->drawTextOnCenter("\n\n.... .", permFontSize, permTextClr, permBackgroundClr);
	} else if(lastCount == 6) {
		screen->drawTextOnCenter("\n\n..... ", permFontSize, permTextClr, permBackgroundClr);
		lastCount = 0;
	}
}

void LoyalityScreen::gotoStateDrawTemp() {
	LOG_DEBUG(LOG_SCREEN, "gotoStateDrawTemp");
	redrawTimer->start(SCREEN_REDRAW_TIMEOUT);
	state = State_DrawTemp;
}

void LoyalityScreen::stateDrawTempTimeout() {
	LOG_DEBUG(LOG_SCREEN, "stateDrawTempTimeout");
	redrawTimer->start(SCREEN_REDRAW_TIMEOUT);
	switch(tempCommand) {
		case Command_drawQrCode: {
			if(procDrawTempQrCode() == true) {
				gotoStateWaitTemp();
				return;
			}
			break;
		}
		default: {
			gotoStateIdle();
			return;
		}
	}
}

void LoyalityScreen::gotoStateWaitTemp() {
	LOG_DEBUG(LOG_SCREEN, "gotoStateWaitTemp");
	redrawTimer->start(SCREEN_TEMP_TIMEOUT);
	state = State_WaitTemp;
}

void LoyalityScreen::stateWaitTempTimeout() {
	LOG_DEBUG(LOG_SCREEN, "stateWaitTempTimeout");
	gotoStateDrawPerm();
}

bool LoyalityScreen::procDrawPermText() {
	if(screen->fillDisplay(permBackgroundClr) == false) { return false; }
	if(screen->drawTextOnCenter(permText.getString(), permFontSize, permTextClr, permBackgroundClr) == false) { return false; }
	return true;
}

bool LoyalityScreen::procDrawPermProgress() {
	if(screen->fillDisplay(permBackgroundClr) == false) { return false; }
	if(screen->drawTextOnCenter(permText.getString(), permFontSize, permTextClr, permBackgroundClr) == false) { return false; }
	return true;
}

bool LoyalityScreen::procDrawPermImage() {
	return screen->showDefaultImage();
}

bool LoyalityScreen::procPermClear() {
	return screen->clearDisplay();
}

bool LoyalityScreen::procDrawTempQrCode() {
	if(screen->drawQR(tempText.getString()) == false) { return false; }
	if(screen->drawText("Кассовый чек", 50, 214, 3, 0, 0xFFFF) == false) { return false; }
	return true;
}
