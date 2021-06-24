#ifndef __RFID_H_
#define __RFID_H_

#include "defines.h"
#include "mfrc532.h"

class RFID: public MFRC532 {
public:
	static RFID *get();

	RFID(I2C *i2c);
	virtual void setResetPin(bool state) override;
	virtual uint8_t wirereadstatus(void) override;

	void testCardRead(int timeout);
//	bool loadCard();
};

#endif
