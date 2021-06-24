#ifndef COMMON_SIM900_TEST_GSMSIGNALQUALITY_H_
#define COMMON_SIM900_TEST_GSMSIGNALQUALITY_H_

#include "sim900/include/GsmSignalQuality.h"

namespace Gsm {

class TestSignalQuality : public SignalQualityInterface {
public:
	TestSignalQuality(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str) {}

	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual bool get() { *str << "<GSM::SQ::get>"; return true; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
};

}

#endif
