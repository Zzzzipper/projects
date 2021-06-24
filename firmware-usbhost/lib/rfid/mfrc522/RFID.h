
#ifndef __RFID_H_
#define __RFID_H_

#include "defines.h"
#include "MFRC522.h"

//#include "common/utils/include/StringBuilder.h"
//#include "common/common.h"


class RFID: public MFRC522
{
private:
	RFID(SPI_TypeDef* spi);
	SPI_TypeDef* spi;


	void sendData(uint8_t data);
	uint8_t receiveData(uint8_t data);

public:
	static RFID *get();

	virtual void PCD_WriteRegister(uint8_t reg, uint8_t value);
	virtual void PCD_WriteRegister(uint8_t reg, uint8_t count, uint8_t *values);
	virtual uint8_t PCD_ReadRegister(uint8_t reg);
	virtual void PCD_ReadRegister(uint8_t reg, uint8_t count, uint8_t *values, uint8_t rxAlign = 0);
	virtual void setResetPin(bool state);

	void setChipEnable(bool enable);

	void testCardRead();
	bool loadCard();
};


#endif
