#include "TestGsmHardware.h"

namespace Gsm {

TestHardware::TestHardware(StringBuilder *result) :
	result(result),
	statusUp(false)
{

}

void TestHardware::setStatusUp(bool statusUp) {
	this->statusUp = statusUp;
}

void TestHardware::init() {
	*result << "<Gsm::Hardware::init>";
}

void TestHardware::pressPowerButton() {
	*result << "<Gsm::Hardware::pressPowerButton>";
}

void TestHardware::releasePowerButton() {
	*result << "<Gsm::Hardware::releasePowerButton>";
}

bool TestHardware::isStatusUp() {
	return statusUp;
}

}
