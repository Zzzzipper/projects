#ifndef NETWORK_ENC28J60_H
#define NETWORK_ENC28J60_H

#include <inttypes.h>

#include "lib/spi/SPI.h"

class ENC28J60 {
public:
	enum DuplexMode {
		Full,
		Half,
		Error
	};

	ENC28J60(SPI *spi);
	void init(uint8_t *macaddr);
	uint16_t recvFrame(uint8_t *frame, uint16_t size);
	void sendFrame(uint8_t *frame, uint16_t len);
	uint8_t enc28j60linkup(void);
	void setHalfDuplexMode();
	void setFullDuplexMode();
	enum DuplexMode getDuplexMode();
	bool checkAnswer();

#if (HW_VERSION == HW_2_0_0)
#elif (HW_VERSION >= HW_3_0_0)
void resetChip(bool state);
#else
#error "HW_VERSION must be defined in project settings"
#endif

private:
	SPI *spi;
	uint8_t bank;
	uint16_t gNextPacketPtr;
	uint8_t erxfcon;

	void enc28j60Init(uint8_t* macaddr);
	inline void enableChip();
	inline void disableChip();
	void delay(int ms);

	uint8_t ENC28J60_SendByte(uint8_t tx);
	uint8_t enc28j60ReadOp(uint8_t op, uint8_t address);
	void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data);
	void enc28j60ReadBuffer(uint16_t len, uint8_t* data);
	void enc28j60WriteBuffer(uint16_t len, uint8_t* data);
	uint16_t enc28j60ReadBufferWord();
	void enc28j60SetBank(uint8_t address);
	uint8_t enc28j60Read(uint8_t address);
	void enc28j60WriteWord(uint8_t address, uint16_t data);
	uint16_t enc28j60PhyRead(uint8_t address);
	void enc28j60Write(uint8_t address, uint8_t data);
	void enc28j60PhyWrite(uint8_t address, uint16_t data);
	void enc28j60clkout(uint8_t clk);
	uint8_t enc28j60hasRxPkt(void);
	uint8_t enc28j60getrev(void);
	void enc28j60EnableBroadcast( void );
	void enc28j60DisableBroadcast( void );
	void enc28j60EnableMulticast( void );
	void enc28j60DisableMulticast( void );
};

#endif
