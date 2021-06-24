#pragma once

#include "lib/rfid/RfidInterface.h"

#include "common/config/include/ConfigModem.h"
#include "common/i2c/I2C.h"
#include "common/timer/include/TimerEngine.h"
#include "common/utils/include/Event.h"
#include "common/fiscal_register/include/QrCodeStack.h"

#define SCREEN_MAX_COMMAND_REPEAT		5

class Screen
{
private:
	Mdb::DeviceContext *context;
	I2C *i2c;
	bool initialized;
	TimerEngine *timerEngine;
	Timer *rfidRequestTimer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	uint8_t lastRfidId[7];

	void procRfidRequestTimer();
	void procClearDisplayTimer();
	uint8_t makeCrc8(uint8_t *pData, uint16_t len);
	bool checkCrc8(uint8_t *pData, uint16_t len);
	bool checkCrcLastTransaction(uint8_t *pData, uint16_t len, bool transmit);
	int getCrcLastTransaction(bool transmit);
	uint32_t makeCrc32(uint8_t *pData, uint32_t len);

	enum class Addr
		{
			Cmd0 = 0, //
			GetRfidId = 2, //
			SetBacklight = 9, //
			FillDisplay = 10, //
			GetCrcSlaveRx = 12, // CRC предыдущей транзакции приема на устройстве
			GetCrcSlaveTx = 16, // CRC предыдущей транзакции передачи на устройстве
			GetSoftwareVersion = 20, //
			GetApplicationStage = 24, //
			GetSoftwareStatus = 25,
			DrawQrString = 32, //
			SetBeepFreq = 50, //
			SetBeepTimeout = 52, //
		GetTouchScreenEventCounter = 100, //
			DrawText_X_Pos = 290, //
			DrawText_Y_Pos = 291, //
			DrawText_TextColor = 292, //
			DrawText_BackgroundColor = 294, //
			DrawText_FontSize = 296, //
			DrawText_Mode = 297, // Бит 0 = 0 - по координатам X и У, иначе автоматическое позиционирование по центру
			DrawText = 298,	 //
			Flash_Cmd = 1000, //
			Flash_CmdStatus = 1001, //
			Flash_DataCount = 1092, //
			Flash_DataCRC = 1096, //
			Flash_Data = 1100, //
		};

		enum class Reg0
		{
			ShowImage1 = 1, //
			ShowImage2 = 2, //
			SetOrientationVertical1 = 10, //
			SetOrientationHorizontal1 = 11, //
			SetOrientationVertical2 = 12, //
			SetOrientationHorizontal2 = 13, //
			Restart = 85 //
		};


		enum class Reg1000
		{
			EraseFirmware = 1, //
			WriteFirmware = 2, //
			EraseExtData = 3, //
			WriteExtData = 4, //
		};

		enum class Reg1001
		{
			successMask = (1 << 0), //
			crcErrorMask = (1 << 1) //
		};

	bool read(const Addr address, uint8_t *data, const int len, int repeat = SCREEN_MAX_COMMAND_REPEAT);
	bool write(const Addr address, uint8_t *data, const int len, int repeat = SCREEN_MAX_COMMAND_REPEAT);

public:
	Screen(Mdb::DeviceContext *context, I2C *i2c, TimerEngine *timerEngine, EventEngineInterface *eventEngine);

	enum class Rotation
	{
		// Взято из проекта Screen/Middlewares/ILI9341/ILI9341_STM32_Driver.h
		Vertical1,
		Horizontal1,
		Vertical2,
		Horizontal2,
	};

	enum class Reg24
	{
		Bootloader = 0xAA, //
		Normal = 0xBB //
	};

	enum class Reg25
	{
		Normal = 1,								// 0, Нормальное состояние. Новой прошивки нет, текущая в порядке.
		EmptyCurrentFirmware = 2,				// 1, Рабочая прошивка отсутствует.
		ErrorCrcCurrentFirmware = 3,			// 2, Обнаружено нарушение контрольной суммы рабочей прошивки.
	};

	enum Color
		: uint16_t
	{
		Black = 0x0000, //
		Navy = 0x000F, //
		DarkGreen = 0x03E0, //
		DarkCyan = 0x03EF, //
		Maroon = 0x7800, //
		Purple = 0x780F, //
		Olive = 0x7BE0, //
		LightGrey = 0xC618, //
		DarkGrey = 0x7BEF, //
		Blue = 0x001F, //
		Green = 0x07E0, //
		Cyan = 0x07FF, //
		Red = 0xF800, //
		Magenta = 0xF81F, //
		Yellow = 0xFFE0, //
		White = 0xFFFF, //
		Orange = 0xFD20, //
		GreenYellow = 0xAFE5, //
		Pink = 0xF81F //
	};

	bool init();
	bool checkInit();
	bool isExists();
	bool drawQR(const char *text);
	bool drawText(const char *text, uint8_t x, uint8_t y, uint8_t fontSize, uint16_t textClr, uint16_t backgroundClr);
	bool drawTextOnCenter(const char *text, uint8_t fontMultipleSize, uint16_t textClr, uint16_t backgroundClr);
	bool clearDisplay();
	bool fillDisplay(uint16_t clr);
	uint8_t *getLastRfidId();
	bool setBacklightBrightness(uint8_t value);
	bool showDefaultImage();

	bool setRotation(enum Rotation value);
	bool getSoftwareVersion(uint32_t &version);
	bool getSoftwareStatus(Reg25 &status);
	bool restart();
	bool beep(uint16_t freq, uint16_t timeout);

	bool eraseFirmware();
	bool eraseExtData();
	bool writeFirmware(uint8_t *data, uint16_t len);
	bool writeExtData(uint8_t *data, uint16_t len);
	bool getApplicationStage(Reg24 &stage);
	bool getTouchScreenEventCounter(uint8_t &value);

private:

	bool eraseFlash(Reg1000 cmd);
	bool writeToFlash(Reg1000 cmd, uint8_t *data, uint16_t len);

};
