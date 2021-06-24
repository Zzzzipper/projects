#include <stdio.h>
#include <string.h>

#include "stm32f4xx_conf.h"
#include "defines.h"
#include "common.h"
#include "config.h"
#include "logger/include/Logger.h"

#include "common/timer/stm32/include/SystemTimer.h"

#include "Screen.h"

#define SCREEN_MANUFACTURER "EFR"
#define SCREEN_MODEL "SCREEN1"
#define SCREEN_I2C_ADDRESS 			(0x70 << 1)
#define SCREEN_SEARCH_TIMEOUT		180	// Время максимально долгих транзакции:
										// - FILL_DISPLAY: 41 млс
										// - DRAW_QR_STRING: 130 млс
#define I2C_MAX_DATA_BUFFER			1024

#define CHECK_INIT_AND_EXIST	if (!checkInit() || !isExists()) return false

//---------------------------------------------------------------------------------------------
Screen::Screen(
	Mdb::DeviceContext *context,
	I2C *i2c,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine
) :
	context(context),
	i2c(i2c),
	initialized(false),
	eventEngine(eventEngine),
	deviceId(eventEngine)
{
	this->context->setManufacturer((uint8_t*)SCREEN_MANUFACTURER, sizeof(SCREEN_MANUFACTURER));
	this->context->setModel((uint8_t*)SCREEN_MODEL, sizeof(SCREEN_MODEL));
	this->context->setState(0);
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	if(timerEngine)
	{
		rfidRequestTimer = timerEngine->addTimer<Screen, &Screen::procRfidRequestTimer>(this);
	}
	else
	{
		rfidRequestTimer = NULL;
	}
}

bool Screen::init()
{
	initialized = true;
	bool current = setRotation(Rotation::Horizontal2) && clearDisplay();
	initialized = current;

	if(initialized)
	{
		rfidRequestTimer->start(SCREEN_RFID_TIMEOUT);
		context->setStatus(Mdb::DeviceContext::Status_Work);
	}
	else
	{
		LOG_WARN(LOG_SCREEN, "Screen not initialized !");
	}

	return initialized;
}

// TODO: Важно: кол-во данных, которые можно считать из регистра, определяется обработчиком самого регистра в экранчике!
// Возможно, стоит переделать, но, непонятно как..
bool Screen::read(const Addr address, uint8_t *data, const int len, int repeat)
{
	if (!i2c->syncReadData(SCREEN_I2C_ADDRESS, static_cast<uint16_t>(address), 2, data, len, SCREEN_SEARCH_TIMEOUT))
	{
		if(repeat > 0)
		{
			return read(address, data, len, repeat - 1);
		}
		return false;
	}

	return true;
}

bool Screen::write(const Addr address, uint8_t *data, const int len, int repeat)
{
	if (!i2c->syncWriteData(SCREEN_I2C_ADDRESS, static_cast<uint16_t>(address), 2, data, len, SCREEN_SEARCH_TIMEOUT))
	{
		LOG_WARN(LOG_SCREEN,
				"Error write transaction, address: " << static_cast<uint16_t>(address) << ", attempts left: " << repeat);

		if (repeat > 0)
			return write(address, data, len, repeat - 1);

		return false;
	}

	if (repeat < 0) return true;

	if (!checkCrcLastTransaction(data, len, true))
	{
		LOG_WARN(LOG_SCREEN, "Error CRC in write transaction, address: " << static_cast<uint16_t>(address) << ", attempts left: " << repeat);

		if (repeat > 0)
			return write(address, data, len, repeat - 1);

		return false;
	}

	return true;
}

bool Screen::checkInit()
{
	return (initialized || init());
}

bool Screen::isExists()
{
	bool exist = i2c->syncWriteData(SCREEN_I2C_ADDRESS, 0, 0, NULL, 0, SCREEN_SEARCH_TIMEOUT);
	if (exist)
		return true;

	LOG_WARN(LOG_SCREEN, "Screen not found");
	return false;
}

bool Screen::drawQR(const char *text)
{
	LOG_DEBUG(LOG_SCREEN, "drawQR");

	CHECK_INIT_AND_EXIST;

	int len = strlen(text) + 1;
	uint8_t *pData = new uint8_t[len];

	strcpy((char *) pData, text);

	if (!write(Addr::DrawQrString, pData, len))
	{
		LOG_WARN(LOG_SCREEN, "Error drawQR");
		delete pData;

		return false;
	}

	delete pData;

	return true;
}

bool Screen::drawText(const char *text, uint8_t x, uint8_t y, uint8_t fontMultipleSize, uint16_t textClr,
		uint16_t backgroundClr)
{
	LOG_DEBUG(LOG_SCREEN, "drawText " << text);

	CHECK_INIT_AND_EXIST;

	int len = strlen(text) + 8 + 1;
	uint8_t *pData = new uint8_t[len];

	pData[0] = x;
	pData[1] = y;
	*((uint16_t *) &(pData[2])) = textClr;
	*((uint16_t *) &(pData[4])) = backgroundClr;
	pData[6] = fontMultipleSize;
	pData[7] = 0x00; // mode


	strcpy((char *) &(pData[8]), text);

	if (!write(Addr::DrawText_X_Pos, pData, len))
	{
		LOG_WARN(LOG_SCREEN, "Error drawText");
		delete pData;
		return false;
	}

	delete pData;
	return true;
}

bool Screen::drawTextOnCenter(const char *text, uint8_t fontMultipleSize, uint16_t textClr, uint16_t backgroundClr)
{
	LOG_DEBUG(LOG_SCREEN, "drawTextOnCenter");

	CHECK_INIT_AND_EXIST;

	int len = strlen(text) + 6 + 1;
	uint8_t *pData = new uint8_t[len];

	*((uint16_t *) &(pData[0])) = textClr;
	*((uint16_t *) &(pData[2])) = backgroundClr;
	pData[4] = fontMultipleSize;
	pData[5] = 0b1; // Mode

	strcpy((char *) &(pData[6]), text);

	if (!write(Addr::DrawText_TextColor, pData, len))
	{
		LOG_WARN(LOG_SCREEN, "Error drawTextOnCenter");
		delete pData;
		return false;
	}

	delete pData;
	return true;
}

bool Screen::clearDisplay()
{
	return fillDisplay(0x0000);
}

bool Screen::fillDisplay(uint16_t clr)
{
	LOG_DEBUG(LOG_SCREEN, "fillDisplay, clr: " << clr);

	CHECK_INIT_AND_EXIST;

	uint8_t data[2];
	data[0] = LOBYTE(clr);
	data[1] = HIBYTE(clr);

	if (!write(Addr::FillDisplay, data, sizeof(data)))
	{
		LOG_WARN(LOG_SCREEN, "Error fillDisplay");
		return false;
	}

	return true;
}

void Screen::procRfidRequestTimer()
{
	rfidRequestTimer->start(SCREEN_RFID_TIMEOUT);
	if (!checkInit() || !isExists()) return;

	uint8_t data[7];

	if (!read(Addr::GetRfidId, data, sizeof(data)))
	{
		LOG_WARN(LOG_SCREEN, "Error procRfidRequestTimer");
		return;
	}

	if (!data[0] && !data[1] && !data[2] && !data[3] && !data[4])
		return;

	if (data[0] == 0xff && data[1] == 0xff && data[2] == 0xff && data[3] == 0xff && data[4] == 0xff)
		return;

	memcpy(lastRfidId, data, 7);
	{
		char str[64];
		sprintf(str, "Rfid id: %.2X %.2X %.2X %.2X %.2X %.2X %.2X", lastRfidId[0], lastRfidId[1], lastRfidId[2], lastRfidId[3], lastRfidId[4], lastRfidId[5], lastRfidId[6]);
		LOG_DEBUG(LOG_SCREEN, str);
	}
	if (eventEngine)
	{
		Rfid::EventCard event(deviceId, lastRfidId, 5);
		eventEngine->transmit(&event);
	}
}

void Screen::procClearDisplayTimer()
{
	clearDisplay();
}

uint8_t *Screen::getLastRfidId()
{
	return lastRfidId;
}

bool Screen::setBacklightBrightness(uint8_t value)
{
	CHECK_INIT_AND_EXIST;

	uint8_t data[1];
	data[0] = value;

	if (!write(Addr::SetBacklight, data, sizeof(data)))
	{
		LOG_WARN(LOG_SCREEN, "Error setBacklightBrightness");
		return false;
	}

	return true;
}

bool Screen::showDefaultImage()
{
	LOG_DEBUG(LOG_SCREEN, "showDefaultImage");

	CHECK_INIT_AND_EXIST;

	uint8_t data[1];
	data[0] = static_cast<uint8_t>(Reg0::ShowImage1);

	if (!write(Addr::Cmd0, data, sizeof(data)))
	{
		LOG_WARN(LOG_SCREEN, "Error showDefaultImage");
		return false;
	}

	return true;
}

bool Screen::setRotation(enum Rotation value)
{
	LOG_DEBUG(LOG_SCREEN, "setRotation");

	CHECK_INIT_AND_EXIST;

	uint8_t data[1];
	data[0] = static_cast<uint8_t>(Reg0::SetOrientationVertical1) + static_cast<uint8_t>(value);

	if (!write(Addr::Cmd0, data, sizeof(data)))
	{
		LOG_WARN(LOG_SCREEN, "Error setRotation");
		return false;
	}

	return true;
}

bool Screen::getSoftwareVersion(uint32_t &version)
{
	LOG_DEBUG(LOG_SCREEN, "getSoftwareVersion");

	CHECK_INIT_AND_EXIST;

	uint8_t *data = (uint8_t *) &version;

	if (!read(Addr::GetSoftwareVersion, data, 4))
	{
		LOG_WARN(LOG_SCREEN, "Error getSoftwareVersion");
		return false;
	}

	return true;
}

bool Screen::getSoftwareStatus(Reg25 &status)
{
	LOG_DEBUG(LOG_SCREEN, "getSoftwareStatus");

	CHECK_INIT_AND_EXIST;

	uint8_t data[1];

	if (!read(Addr::GetSoftwareStatus, data, 1))
	{
		LOG_WARN(LOG_SCREEN, "Error getSoftwareStatus");
		return false;
	}

	if (data[0] == static_cast<uint8_t>(Reg25::Normal))
	{
		status = Reg25::Normal;
		return true;
	}

	if (data[0] ==  static_cast<uint8_t>(Reg25::EmptyCurrentFirmware))
	{
		status = Reg25::EmptyCurrentFirmware;
		return true;
	}

	if (data[0] ==  static_cast<uint8_t>(Reg25::ErrorCrcCurrentFirmware))
	{
		status = Reg25::ErrorCrcCurrentFirmware;
		return true;
	}

	return false;
}

bool Screen::restart()
{
	LOG_DEBUG(LOG_SCREEN, "restart");

	CHECK_INIT_AND_EXIST;

	uint8_t data[1];
	data[0] = static_cast<uint8_t>(Reg0::Restart);

	if (!write(Addr::Cmd0, data, sizeof(data), -1))
	{
		LOG_WARN(LOG_SCREEN, "Error restart");

		return false;
	}

	return true;
}

bool Screen::beep(uint16_t freq, uint16_t timeout)
{
	LOG_DEBUG(LOG_SCREEN, "beep, freq: " << freq << ", timeout: " << timeout);

	CHECK_INIT_AND_EXIST;

	uint16_t data[2];
	data[0] = freq;
	data[1] = timeout;

	if (!write(Addr::SetBeepFreq, (uint8_t *) data, sizeof(data)))
	{
		LOG_WARN(LOG_SCREEN, "Error beep");
		return false;
	}

	return true;
}

bool Screen::getApplicationStage(Reg24 &stage)
{
	LOG_DEBUG(LOG_SCREEN, "getApplicationStage");

	CHECK_INIT_AND_EXIST;

	uint8_t data[1];

	if (!read(Addr::GetApplicationStage, data, 1))
	{
		LOG_WARN(LOG_SCREEN, "Error getSoftwareVersion");
		return false;
	}

	if (data[0] == static_cast<uint8_t>(Reg24::Bootloader))
	{
		stage = Reg24::Bootloader;
		return true;
	}

	if (data[0] ==  static_cast<uint8_t>(Reg24::Normal))
	{
		stage = Reg24::Normal;
		return true;
	}
	return false;
}

bool Screen::getTouchScreenEventCounter(uint8_t &value)
{
	LOG_DEBUG(LOG_SCREEN, "getTouchScreenEventCounter");

	CHECK_INIT_AND_EXIST;

	uint8_t data[1];

	if (!read(Addr::GetTouchScreenEventCounter, data, 1))
	{
		LOG_WARN(LOG_SCREEN, "Error getTouchScreenEventCounter");
		return false;
	}

	value = data[0];

	return true;
}

bool Screen::eraseFirmware()
{
	LOG_DEBUG(LOG_SCREEN, "eraseFirmware");

	return eraseFlash(Reg1000::EraseFirmware);
}

bool Screen::eraseExtData()
{
	LOG_DEBUG(LOG_SCREEN, "eraseExtData");

	return eraseFlash(Reg1000::EraseExtData);
}

bool Screen::eraseFlash(Reg1000 cmd)
{
	CHECK_INIT_AND_EXIST;

	uint8_t data[1];

	data[0] = static_cast<uint8_t>(cmd);

	if (!write(Addr::Flash_Cmd, data, 1, -1))
	{
		LOG_WARN(LOG_SCREEN, "Error eraseFlash, cmd: " << data[0]);

		return false;
	}

	return true;
}

bool Screen::writeFirmware(uint8_t *data, uint16_t len)
{
	return writeToFlash(Reg1000::WriteFirmware, data, len);
}

bool Screen::writeExtData(uint8_t *data, uint16_t len)
{
	return writeToFlash(Reg1000::WriteExtData, data, len);
}

bool Screen::writeToFlash(Reg1000 cmd, uint8_t *data, uint16_t len)
{
	LOG_DEBUG(LOG_SCREEN, "writeFirmware, len: " << len << ", cmd: " << static_cast<uint8_t>(cmd));

	if (len > I2C_MAX_DATA_BUFFER)
	{
		LOG_ERROR(LOG_SCREEN, "Error length parameter, len: " << len << ", max value: " << I2C_MAX_DATA_BUFFER);
		return false;
	}

	CHECK_INIT_AND_EXIST;

	uint32_t tmp32[2];

	tmp32[0] = len;
	tmp32[1] = makeCrc32(data, len);

	if (!write(Addr::Flash_DataCount, (uint8_t *) tmp32, 8))
	{
		LOG_WARN(LOG_SCREEN, "Error set parameters in writeFirmware");

		return false;
	}

	if (!write(Addr::Flash_Data, data, len))
	{
		LOG_WARN(LOG_SCREEN, "Error set data in writeFirmware");

		return false;
	}

	uint8_t tmp8[2];
	tmp8[0] = static_cast<uint8_t>(cmd);

	if (!write(Addr::Flash_Cmd, tmp8, 1, -1))
	{
		LOG_WARN(LOG_SCREEN, "Error writeFirmware");

		return false;
	}

	// Считываем статус выполнения
	if (!read(Addr::Flash_CmdStatus, tmp8, 1))
	{
		LOG_WARN(LOG_SCREEN, "Error read writeFirmware status result");

		return false;
	}

	if (/*tmp8[0] != static_cast<uint8_t>(cmd) ||*/ tmp8[0] != static_cast<uint8_t>(Reg1001::successMask))
	{
		LOG_WARN(LOG_SCREEN, "Error writeFirmware status, cmd: " << tmp8[0] << ", status: " << tmp8[1]);

		return false;
	}

	return true;
}

bool Screen::checkCrcLastTransaction(uint8_t *pData, uint16_t len, bool transmit)
{
#ifndef SCREEN_DISABLE_CHECK_CRC

	uint8_t sourceCrc = makeCrc8(pData, len);
	uint8_t targetCrc = getCrcLastTransaction(transmit) & 0xff;

	if (sourceCrc != targetCrc)
	{
		LOG_ERROR(LOG_SCREEN, "Error CRC, source: " << LOG_HEX(sourceCrc) << ", target: " << LOG_HEX(targetCrc));
		return false;
	}
#endif

	return true;
}

int Screen::getCrcLastTransaction(bool transmit)
{
	uint8_t data[4];
	Addr cmd;
	if (transmit)
		cmd = Addr::GetCrcSlaveRx;
	else
		cmd = Addr::GetCrcSlaveTx;

	if (!read(cmd, data, sizeof(data)))
	{
		LOG_WARN(LOG_SCREEN, "Error getCrcLastTransaction, dir: " << transmit);
		return 0;
	}

	int *res = (int *) data;

	return res[0];
}

uint8_t Screen::makeCrc8(uint8_t *pData, uint16_t len)
{
	uint8_t crc = 0;
	while (len)
	{
		crc += *pData;
		pData++;
		len--;
	}
	return ~crc;
}

bool Screen::checkCrc8(uint8_t *pData, uint16_t len)
{
	uint8_t receivedCrc = pData[len - 1];
	uint8_t crc = makeCrc8(pData, len - 2);
	if (receivedCrc != crc)
	{
		printf("Error CRC, received: %.2X, calculated: %.2X", receivedCrc, crc);
		return false;
	}
	return true;
}

uint32_t Screen::makeCrc32(uint8_t *pData, uint32_t len)
{
	CRC_ResetDR();

	for (uint32_t index = 0; index < len; index++)
		CRC->DR = pData[index];

	return (CRC->DR);
}


#undef CHECK_INIT_AND_EXIST
