#include "extern/wolfssl/wolfssl/wolfcrypt/wc_port.h"

#include "common/timer/qt/QRealTime.h"

time_t XTIME(time_t *timer) {
	QRealTime realtime;
	uint32_t result = realtime.getUnixTimestamp();
	if(timer != 0) { *timer = result; }
	return result;
}
