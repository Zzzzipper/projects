#include <lib/utils/stm32/Reboot.h>
#include "cmsis_boot/stm32f4xx.h"
#include "cmsis_lib/include/stm32f4xx_rcc.h"

void Reboot::reboot() {
	NVIC_SystemReset();
}

/*
@arg RCC_FLAG_BORRST: POR/PDR or BOR reset
@arg RCC_FLAG_PINRST: Pin reset
@arg RCC_FLAG_PORRST: POR/PDR reset
@arg RCC_FLAG_SFTRST: Software reset
@arg RCC_FLAG_IWDGRST: Independent Watchdog reset
@arg RCC_FLAG_WWDGRST: Window Watchdog reset
@arg RCC_FLAG_LPWRRST: Low Power reset

Reset Button:
main.cpp#263 resetBor=0
main.cpp#264 resetPin=1
main.cpp#265 resetPdr=0
main.cpp#266 resetSoftware=0
main.cpp#267 resetIWD=0
main.cpp#268 resetWWD=0
main.cpp#269 resetLowPower=0

WatchDog:
main.cpp#263 resetBor=0
main.cpp#264 resetPin=1
main.cpp#265 resetPdr=0
main.cpp#266 resetSoftware=0
main.cpp#267 resetIWD=1
main.cpp#268 resetWWD=0
main.cpp#269 resetLowPower=0

PowerDown:
main.cpp#263 resetBor=1
main.cpp#264 resetPin=1
main.cpp#265 resetPdr=1
main.cpp#266 resetSoftware=0
main.cpp#267 resetIWD=0
main.cpp#268 resetWWD=0
main.cpp#269 resetLowPower=0

Software:
main.cpp#263 resetBor=0
main.cpp#264 resetPin=1
main.cpp#265 resetPdr=0
main.cpp#266 resetSoftware=1
main.cpp#267 resetIWD=0
main.cpp#268 resetWWD=0
main.cpp#269 resetLowPower=0
 */
Reboot::Reason Reboot::getLastRebootReason() {
	uint8_t resetBor = RCC_GetFlagStatus(RCC_FLAG_BORRST);
	uint8_t resetPdr = RCC_GetFlagStatus(RCC_FLAG_PORRST);
	uint8_t resetSoftware = RCC_GetFlagStatus(RCC_FLAG_SFTRST);
	uint8_t resetIWD = RCC_GetFlagStatus(RCC_FLAG_IWDGRST);
	RCC_ClearFlag();
	if(resetIWD > 0) {
		return Reason_WatchDog;
	} else if(resetPdr > 0 || resetBor > 0) {
		return Reason_PowerDown;
	} else if(resetSoftware > 0){
		return Reason_Software;
	} else {
		return Reason_Button;
	}
}
