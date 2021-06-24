#include "extern/wolfssl/wolfssl/wolfcrypt/wc_port.h"

#include "common/timer/stm32/include/RealTimeControl.h"

time_t XTIME(time_t *timer) {
	uint32_t result = RealTimeControl::get()->getUnixTimestamp();
	if(timer != 0) { *timer = result; }
	return result;
}
