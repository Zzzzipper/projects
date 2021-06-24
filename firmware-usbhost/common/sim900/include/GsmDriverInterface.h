#ifndef COMMON_GSM_DRIVERINTERFACE_H
#define COMMON_GSM_DRIVERINTERFACE_H

namespace Gsm {

class DriverInterface {
public:
	virtual void restart() = 0;
};

}

#endif
