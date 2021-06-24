#ifndef COMMON_GSM_TESTHARDWARE_H
#define COMMON_GSM_TESTHARDWARE_H

#include "sim900/GsmHardware.h"
#include "utils/include/StringBuilder.h"

namespace Gsm {

class TestHardware : public HardwareInterface {
public:
	TestHardware(StringBuilder *result);
	void setStatusUp(bool);

	void init() override;
	void pressPowerButton() override;
	void releasePowerButton() override;
	bool isStatusUp() override;

private:
	StringBuilder *result;
	bool statusUp;
};

}

#endif
