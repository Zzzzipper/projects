#ifndef __UTILS_TESTS
#define __UTILS_TESTS

/** Прочитайте инструкцию в "common/test/include/Test.h"! */
#include "common/test/include/Test.h"

class TestPlatform : public TestSet {
public:
	TestPlatform();
	bool testOperatorNew();
	bool testUart9Bit();
};

void testMain();

#endif
