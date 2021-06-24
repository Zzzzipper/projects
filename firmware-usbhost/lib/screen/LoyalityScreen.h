#pragma once

#include "Screen.h"

#include "lib/rfid/RfidInterface.h"

#include "common/i2c/I2C.h"
#include "common/timer/include/TimerEngine.h"
#include "common/utils/include/Event.h"
#include "common/fiscal_register/include/QrCodeStack.h"

class LoyalityScreen : public ScreenInterface
{
public:
	LoyalityScreen(Screen *screen, TimerEngine *timerEngine);
	void reset();
	bool drawQrCode(const char *header, const char *footer, const char *text) override;
	bool drawText(const char *text, uint8_t fontMultipleSize = 2, uint16_t textClr = 0xFFFF, uint16_t backgroundClr = 0) override;
	bool drawProgress(const char *text, uint8_t fontMultipleSize = 3, uint16_t textClr = 0xFFFF, uint16_t backgroundClr = 0) override;
	bool drawImage() override;
	bool clear() override;

private:
	enum State {
		State_Idle = 0,
		State_DrawPerm,
		State_WaitProgress,
		State_DrawTemp,
		State_WaitTemp,
	};

	enum Command {
		Command_None = 0,
		Command_drawText,
		Command_drawProgress,
		Command_drawQrCode,
		Command_drawImage,
		Command_clear,
	};

	Screen *screen;
	TimerEngine *timerEngine;
	Timer *redrawTimer;
	EventEngineInterface *eventEngine;
	State state;
	Command permCommand;
	StringBuilder permText;
	uint8_t permFontSize;
	uint16_t permTextClr;
	uint16_t permBackgroundClr;
	Command tempCommand;
	StringBuilder tempHeader;
	StringBuilder tempText;
	uint16_t lastCount;

	void procRedrawTimer();
	void gotoStateIdle();
	void gotoStateDrawPerm();
	void stateDrawPermTimeout();
	void gotoStateWaitProgress();
	void stateWaitProgressTimeout();
	void gotoStateDrawTemp();
	void stateDrawTempTimeout();
	void gotoStateWaitTemp();
	void stateWaitTempTimeout();

	bool procDrawPermText();
	bool procDrawPermProgress();
	bool procDrawPermImage();
	bool procPermClear();
	bool procDrawTempQrCode();
};
