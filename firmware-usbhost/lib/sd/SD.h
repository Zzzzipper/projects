#ifndef __SD_H
#define __SD_H

/*
 * Тип: Синглетон
 * Описание: Модуль, для работы с SD карточкой. Режим - 1 бит.
 *
 *
 * Источник примера: http://cxem.net/mc/mc325.php
 */

#include <stdint.h>
#include "common/utils/include/Fifo.h"
#include "defines.h"
#include "common.h"


class SD {
	public:
	  static SD *get();
	  SD();

	  // Наличие SD карты.
	  bool hasCard();


	  bool test();

private:

};

#endif
