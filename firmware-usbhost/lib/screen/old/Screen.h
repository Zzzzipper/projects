#pragma once

#include "i2c/i2c.h"
#include "u8g2/cppsrc/U8g2lib.h"
#include "extern/qrcodegen/qrcodegen.h"

#include "common/timer/include/TimerEngine.h"
#include "common/fiscal_register/include/QrCodeStack.h"

/*
 * Библиотека работы с экраном: https://github.com/olikraus/u8g2
 */
class Screen : public U8G2, public QrCodeInterface {
public:
	Screen(I2C *i2c, TimerEngine *timerEngine);

	uint8_t u8x8_byte_hw_i2c(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);

	bool init();
	bool printQrCode(const char *header, const char *footer, const char *text) override;
	bool printLine1(const char *line1) override;
	void drawQR(const char *text, enum qrcodegen_Mask mask);
	void drawSample();

private:
	I2C *i2c;
	TimerEngine *timerEngine;
	Timer *timer;
	uint8_t i2c_data[64];
	uint16_t i2c_data_len = 0;
	bool enabled;

	void procTimer();
	void printQr(const uint8_t qrcode[]);
};
