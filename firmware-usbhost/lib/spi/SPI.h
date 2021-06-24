#ifndef __SPI_H
#define __SPI_H


/*
 * Тип: Синглетон
 * Описание: Модуль, для работы с SPI в синхронном режиме
 *
 * FIXME: Доделать модуль SPI для асинхронного режима. Желательно, через DMA, по аналогии с I2C
 *
 */

#include <stdint.h>
#include "common/utils/include/Fifo.h"
#include "defines.h"
#include "common.h"

enum enSPI {
	SPI_1,
	SPI_2,
	SPI_3
};

class SPI {
	public:
	  static SPI *get(enum enSPI index);
	  SPI(enum enSPI index);
	  ~SPI();
	  void receive_isr(uint8_t data);

	  void sendData(uint8_t data);
	  uint8_t receiveData(uint8_t data);
	  void setChipEnable(bool enable);

	private:
	  enum enSPI index;
	  SPI_TypeDef* spi;
	  Fifo<uint8_t> *receiveBuffer;

	  void config();
};

#endif
