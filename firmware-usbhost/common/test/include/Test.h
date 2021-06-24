#ifndef COMMON_TEST_H_
#define COMMON_TEST_H_

#include "utils/include/StringBuilder.h"
#include "utils/include/Buffer.h"
#include "utils/include/List.h"
#include "utils/include/Hex.h"
#include "logger/include/LogTargetRam.h"
#include "logger/include/Logger.h"

#include <string.h>

/**
 * Что дает новый движок?
 * 1. Разработчики пишут тесты в отдельных файлах
 * 2. Файлы тестов добавляются в проект и этого достаточно, чтобы тест зарегистрировался в движке тестов
 * 3. Одним вызовом можно запустить все тесты проекта. Нет необходимости искать файлы, руками править
 *    какой-то централизованный файл. Все тесты доступны в любом месте одновременно и их можно запустить
 *    все одним вызовом.
 *
 * Требования к тестам:
 * 1. Тесты должны быть автономными, то есть не должны требовать внешней инициализации.
 * 2. Тест должен использовать для проверок только макросы движка и не должен требовать от пользователям
 *    что-то проверять глазами и сравнивать.
 *
 * Как использовать новый движок тестов:
 * 1. Создайте набор тестов на базе TestSet:
 * MyTest.h
 * class TestMySet : public TestSet {
 * public:
 *     TestPlatform();
 *	   void init(); // вызывается перед каждым тестом. Рекомендуется инициализация общая для testMyCase1 и testMyCase2
 *	   void cleanup(); // вызывается после каждого теста. Рекомендуется освобождение ресурсов общее для testMyCase1 и testMyCase2
 *     bool testMyCase1();
 *     bool testMyCase2();
 * }
 *
 * 2. Напишите тестовые случаи и зарегистрируйте их в движке:
 * MyTest.cpp
 * #include "MyTest.h"
 *
 * TEST_SET_REGISTER(TestMySet); // регистрирует набор тестов в движке тестов
 *
 * TestMySet::TestMySet() {
 *     TEST_CASE_REGISTER(TestMySet, testMyCase); // регистрируем тестовый случай
 * }
 *
 * bool TestMySet::testMyCase() {
 *     uint16_t value = 2 + 2;
 *     TEST_NUMBER_EQUAL(4, value); // проверяет равенство: ожидается 4, проверяемое значение value
 *     return true;
 * }
 *
 * 3. Выполняйте тесты в любом порядке в любом удобном месте:
 * Tests.cpp
 * #include "common/test/include/Test.h"
 *
 * void main() {
 *     TestEngine::test(); // запустит все зарегистрированные тесты
 *     TestEngine::test("TestMySet"); // запустит все тесты набора TestMySet
 *     TestEngine::test("TestMySet", "testMyCase"); // запустит конркетный тест TestMySet::testMyCase
 * }
 */

/**
 * Сравнивает числа.
 * Параметры:
 *   expected - ожидаемое значение;
 *   actual - фактическое (проверяемое) значение.
 * Результат:
 *   значения совпадают - тест продолжится;
 *   значения не совпадают - тест будет прерван на этом макросе и будет выведено сообщение об ошибке.
 */
#define TEST_NUMBER_EQUAL(expected, actual) \
{ \
	int64_t actualValue = actual; \
	if(expected != actualValue) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << " expected=" << expected << ", actual=" << actualValue); \
		return false; \
	} \
}

/**
 * Сравнивает числа. Ожидает, что значения не должны совпадать.
 * Параметры:
 *   expected - ожидаемое значение;
 *   actual - фактическое (проверяемое) значение.
 * Результат:
 *   значения не совпадают - тест продолжится;
 *   значения совпадают - тест будет прерван на этом макросе и будет выведено сообщение об ошибке.
 */
#define TEST_NUMBER_NOT_EQUAL(expected, actual) \
{ \
	int64_t actualValue = actual; \
	if(expected == actualValue) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << " expected=" << expected << ", actual=" << actualValue); \
		return false; \
	} \
}

#define TEST_POINTER_NOT_NULL(actual) \
{ \
	if(actual == NULL) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << " actual is NULL"); \
		return false; \
	} \
}

#define TEST_POINTER_IS_NULL(actual) \
{ \
	if(actual != NULL) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << " actual is NULL"); \
		return false; \
	} \
}

/**
 * Сравнивает дату и время.
 * Параметры:
 *   expected - ожидаемое значение;
 *   actual - фактическое (проверяемое) значение.
 * Результат:
 *   значения не совпадают - тест продолжится;
 *   значения совпадают - тест будет прерван на этом макросе и будет выведено сообщение об ошибке.
 */
#define TEST_DATETIME_EQUAL(expected, actual) \
{ \
	StringBuilder str; \
	datetime2string(actual, &str); \
	if(testCompareStr(expected, strlen(expected), str.getString(), str.getLen()) == false) { \
		LOG("error in " << __FILE__ << ":" << __LINE__); \
		return false; \
	} \
}

/**
 * Сравнивает строки.
 * Параметры:
 *   expected - ожидаемое значение в шестнадцатиричном формате;
 *   actual - фактическое (проверяемое) значение.
 * Результат:
 *   значения совпадают - тест продолжится;
 *   значения не совпадают - тест будет прерван на этом макросе и будет выведено сообщение об ошибке.
 */
#define TEST_STRING_EQUAL(expected, actual) \
if(testCompareStr(expected, strlen(expected), actual, strlen(actual)) == false) { \
	LOG("error in " << __FILE__ << ":" << __LINE__); \
	return false; \
}

/**
 * Сравнивает строки.
 * Параметры:
 *   expected - ожидаемое значение;
 *   actual - фактическое (проверяемое) значение;
 *   actualLen - длина проверяемой строки.
 * Результат:
 *   значения совпадают - тест продолжится;
 *   значения не совпадают - тест будет прерван на этом макросе и будет выведено сообщение об ошибке.
 */
#define TEST_SUBSTR_EQUAL(expected, actual, actualLen) \
if(testCompareStr(expected, strlen(expected), actual, actualLen) == false) { \
	LOG("error in " << __FILE__ << ":" << __LINE__); \
	return false; \
}

/**
 * Сравнивает бинарные данные, представленные в шестнадцатиричном виде.
 * Параметры:
 *   expected - ожидаемое значение в шестнадцатиричном формате;
 *   actual - фактическое (проверяемое) значение;
 *   actualLen - длина проверяемой строки.
 * Результат:
 *   значения совпадают - тест продолжится;
 *   значения не совпадают - тест будет прерван на этом макросе и будет выведено сообщение об ошибке.
 */
#define TEST_HEXDATA_EQUAL(expected, actual, actualLen) \
{ \
	uint32_t stringLen = actualLen * 2; \
	StringBuilder str(stringLen, stringLen); \
	if(dataToHex(actual, actualLen, &str) == false) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << " data convertion to hex failed"); \
		return false; \
	} \
	if(strcmp(expected, str.getString()) != 0) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << "\r\nexpected=" << expected << "\r\nactual  =" << str.getString()); \
		return false; \
	} \
}

/**
 * Сравнивает потоки данных, где первый байт - это девятый бит, а второй байт - данные.
 * Параметры:
 *   expected - ожидаемое значение в шестнадцатиричном формате;
 *   actual - фактическое (проверяемое) значение;
 *   actualLen - длина проверяемой строки.
 * Результат:
 *   значения совпадают - тест продолжится;
 *   значения не совпадают - тест будет прерван на этом макросе и будет выведено сообщение об ошибке.
 */
#define TEST_MDBDATA_EQUAL(expected, actual, actualLen) \
{ \
	uint32_t stringLen = actualLen * 2; \
	StringBuilder str(stringLen, stringLen); \
	if(mdb2hex(actual, actualLen, &str) == false) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << " data convertion to hex failed"); \
		return false; \
	} \
	if(strcmp(expected, str.getString()) != 0) { \
		LOG("error in " << __FILE__ << ":" << __LINE__ << " expected=" << expected << ", actual=" << str.getString()); \
		return false; \
	} \
}

class TestEcho {
public:
	static TestEcho *get();
	static StringBuilder &stream() { return get()->str; }
	static void clear() { get()->str.clear(); }
	static const char *getString() { return get()->str.getString(); }
	static uint16_t getStringLen() { return get()->str.getLen(); }

private:
	static TestEcho *instance;
	StringBuilder str;
};

class TestCase {
public:
	virtual ~TestCase() {}
	virtual const char *getName() = 0;
	virtual void setName(const char *name) = 0;
	virtual bool test() = 0;
};

template<typename T>
class TestMethod : public TestCase {
public:
	TestMethod(T *context, bool (T::*method)()) : context(context), method(method) {}
	const char *getName() { return name.getString(); }
	void setName(const char *name) { this->name = name; }
	bool test() { return (context->*method)(); }

private:
	StringBuilder name;
	T *context;
	bool (T::*method)();
};

class TestSet {
public:
	virtual ~TestSet() {}
	const char *getName();
	void setName(const char *name);
	void add(const char *testId, TestCase *test);
	uint16_t getLen();
	TestCase *get(uint16_t i);
	TestCase *get(const char *methodName);
	void sort();
	virtual bool init() { return true; }
	virtual void cleanup() {}

private:
	StringBuilder name;
	List<TestCase> tests;
};

class TestEngine {
public:
	static TestEngine *get();

	void add(const char *setId, TestSet *set);
	void sort();
	void test(bool allLog = false);
	void test(const char *setName, bool allLog = false);
	void test(const char *setName, const char *methodName);

private:
	static TestEngine *instance;
	List<TestSet> tests;
	LogTargetRam ramTarget;
	Buffer ramBuffer;
	uint16_t num;
	bool sorted;

	TestEngine();
	TestEngine(const TestEngine &copy);
	TestEngine& operator=(const TestEngine &c);

	TestSet *get(const char *setName);
	bool runTest(TestSet *set, TestCase *test, bool testLog);
	void initErrorList();
	void showErrorList();
};

/**
 * FIRE: На платофрме stm32 не просто не работает, а приводит к зависанию прошивки до входа в main.
 * Почему-то не вызывается конструктор статической глобальной переменной.
 */
template <typename T>
class TestRegister {
public:
	TestRegister(const char *name) {
		m_set = new T;
		TestEngine::get()->add(name, m_set);
	}
	~TestRegister() {
		delete m_set;
	}

private:
	T *m_set;
};

#ifdef TEST
#define STRINGIFY(x) #x
#define MERGE_(prefix, l)  prefix ## l
#define LABEL_(l) MERGE_(id_, l)
#define UNIQUE_NAME LABEL_(__LINE__)
#define TEST_SET_REGISTER(_testClass) static TestRegister<_testClass> UNIQUE_NAME(STRINGIFY(_testClass))
#define TEST_CASE_REGISTER(_testClass, _testMethod) add(STRINGIFY(_testMethod), new TestMethod<_testClass>(this, &_testClass::_testMethod))
#else
#define TEST_SET_REGISTER(_testClass)
#define TEST_CASE_REGISTER(_testClass, _testMethod)
#endif

extern bool testCompareStr(const char *expected, uint16_t expectedLen, const char *actual, uint16_t actualLen);
extern bool mdb2hex(const uint8_t *data, uint16_t dataLen, StringBuilder *str);

#endif
