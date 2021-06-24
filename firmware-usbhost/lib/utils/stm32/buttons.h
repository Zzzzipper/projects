
#ifndef __BUTTONS_H
#define __BUTTONS_H

#include "common/buttons/include/ButtonEngine.h"

class Buttons : public ButtonEngine {
public:
	enum EventType {
		Event_Button1	= GlobalId_Buttons | 0x01, // uint8_t true - нажата, false - отпущена
		Event_Button2	= GlobalId_Buttons | 0x02,
		Event_Button3	= GlobalId_Buttons | 0x03,
		Event_Button4	= GlobalId_Buttons | 0x04,
		Event_Button5	= GlobalId_Buttons | 0x05,
		Event_Button6	= GlobalId_Buttons | 0x06,
	};

	Buttons();
	~Buttons();

private:
	void initButtons() override;
	void shutdownButtons() override;
};

#endif
