#include "Battery.h"

#include "lib/adc/Adc.h"

#include "common/logger/include/Logger.h"

namespace Battery {

void init() {
#if (HW_VERSION >= HW_3_1_0)
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = BATTERY_PIN;
	GPIO_Init(BATTERY_PORT, &gpio);
#endif
}

void enable() {
#if (HW_VERSION >= HW_3_1_0)
	BATTERY_ENABLE
#endif
}

void disable() {
#if (HW_VERSION >= HW_3_1_0)
	BATTERY_DISABLE
	while(1); // останов выполнения, чтобы не наплодить битых событий
#endif
}

}
