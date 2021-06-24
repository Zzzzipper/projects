#ifndef COMMON_GSM_HARDWARE_H
#define COMMON_GSM_HARDWARE_H

namespace Gsm {

class HardwareInterface {
public:
	virtual ~HardwareInterface() {}
	virtual void init() = 0;
	virtual void pressPowerButton() = 0;
	virtual void releasePowerButton() = 0;
	virtual bool isStatusUp() = 0;
};

class Hardware : public HardwareInterface {
public:
	void init() override;
	void pressPowerButton() override;
	void releasePowerButton() override;
	bool isStatusUp() override;
};

}

#endif
