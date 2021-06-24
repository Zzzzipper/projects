#include <QCoreApplication>

#include "test/include/Test.h"
#include "timer/qt/QRealTime.h"
#include "logger/include/LogTargetStdout.h"
#include "logger/include/Logger.h"

unsigned long HardwareVersion = 1;
unsigned long SoftwareVersion = 1;

int main() {
	LogTargetStdout logTarget;
	Logger::get()->registerTarget(&logTarget);
	QRealTime realtime;
	Logger::get()->registerRealTime(&realtime);

//	TestEngine::get()->test(false);
	TestEngine::get()->test("SaleManagerExeMasterTest", false);
//	TestEngine::get()->test("", "");
	return 0;
}
