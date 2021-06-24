#include "test/include/Test.h"
#include "logger/include/Logger.h"

#define TEST_STRING_RESULT_MAX_SIZE 40

TestEcho *TestEcho::instance = NULL;

TestEcho *TestEcho::get() {
	if(instance == NULL) {
		instance = new TestEcho();
	}
	return instance;
}

const char *TestSet::getName() {
	return name.getString();
}

void TestSet::setName(const char *name) {
	this->name.set(name);
}

void TestSet::add(const char *testId, TestCase *test) {
	test->setName(testId);
	tests.add(test);
}

uint16_t TestSet::getLen() {
	return tests.getSize();
}

TestCase *TestSet::get(uint16_t i) {
	return tests.get(i);
}

TestCase *TestSet::get(const char *methodName) {
	for(uint16_t i = 0; i < tests.getSize(); i++) {
		TestCase *test = tests.get(i);
		if(strcmp(test->getName(), methodName) == 0) {
			return test;
		}
	}
	return NULL;
}

void TestSet::sort() {
	class TestCaseComparator : public List<TestCase>::Comparator {
	public:
		int compare(TestCase *f, TestCase *s) {
			return strcasecmp(f->getName(), s->getName());
		}
	} comparator;
	tests.sort(&comparator);
}

TestEngine *TestEngine::instance = NULL;

TestEngine *TestEngine::get() {
	if(instance == NULL) {
		instance = new TestEngine();
	}
	return instance;
}

void TestEngine::add(const char *id, TestSet *set) {
	set->setName(id);
	tests.add(set);
}

void TestEngine::sort() {
	if(sorted == true) {
		return;
	}

	class TestSetComparator : public List<TestSet>::Comparator {
	public:
		int compare(TestSet *f, TestSet *s) {
			return strcasecmp(f->getName(), s->getName());
		}
	} comparator;
	tests.sort(&comparator);

#if 0
	for(uint16_t i = 0; i < tests.getSize(); i++) {
		tests.get(i)->sort();
	}
#endif

	sorted = true;
}

void TestEngine::test(bool allLog) {
	sort();
	initErrorList();

	*(Logger::get()) << "========== GLOBAL TEST START ===========" << Logger::endl;

	uint32_t global = 0;
	uint32_t globalFailed = 0;
	for(uint16_t i = 0; i < tests.getSize(); i++) {
		TestSet *set = tests.get(i);
		if(allLog == false) { *(Logger::get()) << set->getName() << " "; }

		uint32_t total = 0;
		uint32_t totalFailed = 0;
		for(uint16_t j = 0; j < set->getLen(); j++) {
			TestCase *test = set->get(j);
			bool result = runTest(set, test, allLog);
			if(allLog == false) { *(Logger::get()) << (result == true ? "." : "F"); }
			if(result == false) { totalFailed++; }
			total++;
		}

		if(allLog == false) { *(Logger::get()) << " " << (totalFailed == 0 ? "OK" : "FAIL") << Logger::endl; }
		global += total;
		globalFailed += totalFailed;
	}

	if(globalFailed == 0) {
		*(Logger::get()) << "========== GLOBAL TEST COMPLETE (SUCCEED " << global << " from " << global << ") ============" << Logger::endl;
	} else {
		*(Logger::get()) << "========== GLOBAL TEST COMPLETE (FAILED " << globalFailed << " from " << global << ") ============" << Logger::endl;
	}

	showErrorList();
}

void TestEngine::test(const char *setName, bool allLog) {
	TestSet *set = get(setName);
	if(set == NULL) {
		*(Logger::get()) << "Test set " << setName << " not found" << Logger::endl;
		return;
	}

	sort();
	initErrorList();

	*(Logger::get()) << "========== " << setName << " START ===========" << Logger::endl;

	uint32_t total = 0;
	uint32_t totalFailed = 0;
	for(uint16_t j = 0; j < set->getLen(); j++) {
		TestCase *test = set->get(j);
		bool result = runTest(set, test, allLog);
		*(Logger::get()) << setName << "::" << test->getName() << " " << (result == true ? "OK" : "FAIL") << Logger::endl;
		if(result == false) { totalFailed++; }
		total++;
	}

	if(totalFailed == 0) {
		*(Logger::get()) << "========== " << setName << " COMPLETE (SUCCEED " << total << " from " << total << ") ============" << Logger::endl;
	} else {
		*(Logger::get()) << "========== " << setName << " COMPLETE (FAILED " << totalFailed << " from " << total << ") ============" << Logger::endl;
	}

	showErrorList();
}

void TestEngine::test(const char *setName, const char *methodName) {
	TestSet *set = get(setName);
	if(set == NULL) {
		*(Logger::get()) << "Test set " << setName << " not found" << Logger::endl;
		return;
	}
	TestCase *test = set->get(methodName);
	if(test == NULL) {
		*(Logger::get()) << "Test method " << setName << "::" << methodName << " not found" << Logger::endl;
		return;
	}

#if 0
	*(Logger::get()) << "=======================================" << Logger::endl;
	*(Logger::get()) << setName << "::" << methodName << " START" << Logger::endl;
	*(Logger::get()) << "=======================================" << Logger::endl;
	set->init();
	bool result = test->test();
	set->cleanup();
	*(Logger::get()) << "=======================================" << Logger::endl;
	*(Logger::get()) << setName << "::" << methodName << " " << (result == true ? "OK" : "FAIL") << Logger::endl;
	*(Logger::get()) << "=======================================" << Logger::endl;
#else
	runTest(set, test, true);
#endif
}

TestEngine::TestEngine() : ramTarget(1000), ramBuffer(5000), sorted(false) {}

TestEngine::TestEngine(const TestEngine &) : ramTarget(1000), ramBuffer(5000), sorted(false) {}

TestEngine& TestEngine::operator=(const TestEngine &) { return *this; }

TestSet *TestEngine::get(const char *setName) {
	for(uint16_t i = 0; i < tests.getSize(); i++) {
		TestSet *set = tests.get(i);
		if(strcmp(set->getName(), setName) == 0) {
			return set;
		}
	}
	return NULL;
}

bool TestEngine::runTest(TestSet *set, TestCase *test, bool testLog) {
#if 0
	Logger *logger = Logger::get();
	LogTarget *logTarget = logger->getTarget();
	ramTarget.clear();
	logger->registerTarget(&ramTarget);

	if(testLog == false) { *(Logger::get()) << Logger::endl; }
	*(Logger::get()) << num << "). " << set->getName() << "::" << test->getName() << Logger::endl;
	if(testLog == false) { logger->registerTarget(NULL); }

	set->init();
	bool result = test->test();
	set->cleanup();

	logger->registerTarget(logTarget);
	if(result == false) {
		num++;
		ramBuffer.add(ramTarget.getData(), ramTarget.getLen());
	}
	return result;
#else
	Logger *logger = Logger::get();
	LogTarget *logTarget = logger->getTarget();
	if(testLog == false) {
		ramTarget.clear();
		logger->registerTarget(&ramTarget);
		*(Logger::get()) << Logger::endl;
		*(Logger::get()) << num << "). " << set->getName() << "::" << test->getName() << Logger::endl;
		logger->registerTarget(NULL);
	} else {
		*(Logger::get()) << "=======================================" << Logger::endl;
		*(Logger::get()) << set->getName() << "::" << test->getName() << " START" << Logger::endl;
		*(Logger::get()) << "=======================================" << Logger::endl;
	}

	bool result = false;
	if(set->init() == true) {
		if(test->test() == true) { result = true; }
		set->cleanup();
	}

	if(testLog == false) {
		logger->registerTarget(logTarget);
		if(result == false) {
			num++;
			ramBuffer.add(ramTarget.getData(), ramTarget.getLen());
		}
	} else {
		*(Logger::get()) << "=======================================" << Logger::endl;
		*(Logger::get()) << set->getName() << "::" << test->getName() << " " << (result == true ? "OK" : "FAIL") << Logger::endl;
		*(Logger::get()) << "=======================================" << Logger::endl;
	}

	return result;
#endif
}

void TestEngine::initErrorList() {
	num = 1;
	ramBuffer.clear();
}

void TestEngine::showErrorList() {
	Logger::get()->str(ramBuffer.getData(), ramBuffer.getLen());
}

void printErrorFragment(const char *prefix, const char *str, uint16_t strLen, uint16_t lineNum, uint16_t linePos, uint16_t errPos) {
	uint16_t i = errPos;
	uint16_t endPos = i + TEST_STRING_RESULT_MAX_SIZE;
	for(; i < strLen && i < endPos; i++)  {
		if(str[i] == '\r' || str[i] == '\n') {
			break;
		}
	}
	*(Logger::get()) << prefix << lineNum << ":" << (errPos - linePos) << "/" << errPos << "]:";
	Logger::get()->str((str + linePos), i - linePos);
	*(Logger::get()) << Logger::endl;
}

bool testCompareStr(const char *expected, uint16_t expectedLen, const char *actual, uint16_t actualLen) {
	uint16_t line = 0;
	uint16_t lineStart = 0;
	bool newLine = false;
	uint16_t i = 0;
	for(; i < expectedLen && i < actualLen; i++) {
		if(expected[i] == '\r' || expected[i] == '\n') {
			newLine = true;
		} else {
			if(newLine == true) {
				line++;
				lineStart = i;
				newLine = false;
			}
		}
		if(expected[i] != actual[i]) {
			i++;
			printErrorFragment("expected [", expected, expectedLen, line, lineStart, i);
			printErrorFragment("actual   [", actual, actualLen, line, lineStart, i);
			return false;
		}
	}
	if(expectedLen != actualLen) {
		printErrorFragment("expected [", expected, expectedLen, line, lineStart, i);
		printErrorFragment("actual   [", actual, actualLen, line, lineStart, i);
		return false;
	}
	return true;
}

bool mdb2hex(uint8_t *data, uint16_t dataLen, StringBuilder *str) {
	const char a[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for(uint16_t i = 0; i < dataLen; i++) {
		switch(data[i]) {
			case 0: *str << "d"; break;
			case 1: *str << "a"; break;
			default: {
				*str << "wrong address flag";
				return false;
			}
		}
		if(i >= dataLen) {
			*str << "not found data byte";
			return false;
		}
		i++;
		*str << a[data[i] >> 4] << a[data[i] & 0x0F];
	}
	return true;
}
