#include "include/Button.h"

bool Button::isChange() {
	bool current = checkPressed();
	if(pressed == current) {
		return false;
	} else {
		pressed = current;
		return true;
	}
}
