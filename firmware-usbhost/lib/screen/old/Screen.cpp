#include "Screen.h"

#include "stm32f4xx_conf.h"
#include "defines.h"
#include "common.h"
#include "config.h"
#include "logger/include/Logger.h"

#include "common/timer/stm32/include/SystemTimer.h"

#include <stdio.h>
#include <string.h>

#define SCREEN_I2C_ADDRESS 0x78
#define SCREEN_SEARCH_TIMEOUT 10
#define QRCODE_TIMEOUT 60000
#define QRCODE_BLACK

//---------------------------------------------------------------------------------------------
extern "C" uint8_t u8x8_byte_arduino_hw_i2c(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr) {
	Screen *instance = static_cast<Screen *> (u8x8->tag_object);
	if(!instance) {
		LOG_ERROR(LOG_SCREEN, "Screen object is empty!");
		return 0;
	}

	return instance->u8x8_byte_hw_i2c(u8x8, msg, arg_int, arg_ptr);
}

extern "C" uint8_t u8x8_gpio_and_delay_arduino(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, U8X8_UNUSED void *arg_ptr) {
	LOG_DEBUG(LOG_SCREEN, "gpio_and_delay, msg: " << msg);
	return 1;
}

//---------------------------------------------------------------------------------------------
Screen::Screen(I2C *i2c, TimerEngine *timerEngine) :
	U8G2(),
	i2c(i2c),
	enabled(false)
{
	if(timerEngine) {
		timer = timerEngine->addTimer<Screen, &Screen::procTimer>(this);
	} else {
		timer = NULL;
	}

//todo: потенциал для оптимизации занимаемой памяти
//	u8g2_Setup_ssd1327_i2c_midas_128x128_1(getU8g2(), U8G2_R0, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
//	u8g2_Setup_ssd1327_i2c_midas_128x128_2(getU8g2(), U8G2_R0, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
	u8g2_Setup_ssd1327_i2c_midas_128x128_f(getU8g2(), U8G2_R0, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);

	setI2CAddress(SCREEN_I2C_ADDRESS);
	setTagObject(this);
}

bool Screen::init() {
	if(i2c->syncWriteData(SCREEN_I2C_ADDRESS, 0, 0, 0, 0, SCREEN_SEARCH_TIMEOUT) == false) {
		LOG_ERROR(LOG_SCREEN, "Screen not found");
		return false;
	}

	begin();
	enabled = true;
	return true;
}

bool Screen::printQrCode(const char *header, const char *footer, const char *text) {
	LOG_DEBUG(LOG_SCREEN, "printQrCode");
	if(enabled == false) { return false; }
	drawQR(text, qrcodegen_Mask_AUTO);
	if(timer) timer->start(QRCODE_TIMEOUT);
	return true;
}

bool Screen::printLine1(const char *line1) {
	LOG_DEBUG(LOG_SCREEN, "printLine1");
	if(enabled == false) { return false; }
	return true;
}

void Screen::procTimer() {
	LOG_DEBUG(LOG_SCREEN, "procTimer");
	if(enabled == false) { return; }
	clearDisplay();
}

void Screen::drawQR(const char *text, enum qrcodegen_Mask mask) {
	LOG_DEBUG(LOG_SCREEN, "drawQR");
	if(enabled == false) { return; }

	// Make and print the QR Code symbol
	qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level
	uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(5)]; // todo: в define'ах уменьшить размер версии до 5
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(5)];
	if(qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl, 1, 5, mask, true) == false) {
		LOG_DEBUG(LOG_SCREEN, "qrcodegen_encodeText failed");
		return;
	}

	int size = qrcodegen_getSize(qrcode);
	int mult = 128/size;
	int offset = (128%size)/2;

	LOG_DEBUG(LOG_SCREEN, "Size: " << size << ", multiple: " << mult);

	firstPage();

#ifdef QRCODE_BLACK // черным по белому
	setColorIndex(1);
	for(int y = 0; y < 128; y++) {
		for(int x = 0; x < 128; x++) {
			drawPixel(x, y);
		}
	}

	do {
		for(int y = 0; y < size; y++) {
			for(int x = 0; x < size; x++) {
				if(qrcodegen_getModule(qrcode, x, y)) {
					setColorIndex(0);
					drawBox(x * mult + offset, y * mult + offset, mult, mult);
				}
			}
		}
	} while(nextPage());
#else // белым по черному
	setColorIndex(0);
	for(int y = 0; y < 128; y++) {
		for(int x = 0; x < 128; x++) {
			drawPixel(x, y);
		}
	}

	do {
		for(int y = 0; y < size; y++) {
			for(int x = 0; x < size; x++) {
				if(qrcodegen_getModule(qrcode, x, y)) {
					setColorIndex(1);
					drawBox(x * mult + offset, y * mult + offset, mult, mult);
				}
			}
		}
	} while(nextPage());
#endif
}

void Screen::printQr(const uint8_t qrcode[]) {
	int size = qrcodegen_getSize(qrcode);
	int border = 4;
	for(int y = -border; y < size + border; y++) {
		for (int x = -border; x < size + border; x++) {
			fputs((qrcodegen_getModule(qrcode, x, y) ? "##" : "  "), stdout);
		}
		SystemTimer::get()->delay_ms(5);
		fputs("\r\n", stdout);
	}
	fputs("\r\n", stdout);
}

void Screen::drawSample()
{
	firstPage();

	int pages = 0;

	do {
		setFont(u8g2_font_ncenB14_tr);
		setColorIndex(1);
		drawBox(10, 12, 20, 38);

		setFont(u8g2_font_osb35_tr);
		drawStr(40, 50, "A");

		setFont(u8g2_font_unifont_t_cyrillic);
		drawStr(30, 80, "Company:");

		setFont(u8g2_font_8x13_t_cyrillic);
		drawStr(30, 110, "Sambery !");

		setColorIndex(0);
		drawPixel(28, 14);

		setColorIndex(1);
		drawPixel(20, 13);
		drawPixel(19, 12);
		drawPixel(18, 11);
		drawPixel(17, 10);
		drawPixel(16, 9);
		drawPixel(15, 8);
		drawPixel(14, 7);
		drawPixel(13, 6);
		drawPixel(12, 5);
		drawPixel(11, 4);
		drawPixel(10, 3);
		drawPixel(9, 2);

		for(int coord = 0; coord < 128; coord++) {
			drawPixel(coord, 0);
			drawPixel(coord, 127);
			drawPixel(0, coord);
		  	drawPixel(127, coord);
		}
		pages++;
	} while ( nextPage() );

	LOG_DEBUG(LOG_SCREEN, "Screen pages: " << pages);
}

uint8_t Screen::u8x8_byte_hw_i2c(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
	switch(msg) {
		case U8X8_MSG_BYTE_SEND: {
			if(i2c_data_len + arg_int > sizeof(i2c_data)) {
				LOG_ERROR(LOG_SCREEN, "Overflow SCREEN I2C transmitted data!");
				return 0;
			}

			uint8_t *b = (uint8_t *) arg_ptr;
			for(int i = 0; i < arg_int; i++)
			i2c_data[i2c_data_len++] = b[i];
		}
		break;

		case U8X8_MSG_BYTE_INIT:
			break;

		case U8X8_MSG_BYTE_SET_DC:
			break;

		case U8X8_MSG_BYTE_START_TRANSFER:
			i2c_data_len = 0;
			break;

		case U8X8_MSG_BYTE_END_TRANSFER:
			if(!i2c->syncWriteData(u8x8_GetI2CAddress(u8x8), 0, 0, i2c_data, i2c_data_len, 100))
			{
				LOG_DEBUG(LOG_SCREEN, "Error SCREEN I2C transaction!");
				return 0;
			}
			break;

		default:
			return 0;
	}
	return 1;
}
