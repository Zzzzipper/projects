#ifndef UTILS_RESET_H
#define UTILS_RESET_H

class Reboot {
public:
	enum Reason {
		Reason_Button = 0,
		Reason_PowerDown,
		Reason_WatchDog,
		Reason_Software,
	};

	static void reboot();
	static Reason getLastRebootReason();
};

#endif
