#include "utils/include/sha1.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

//todo: переписать эти тесты
// Все тесты должны быть реализованы в виде модульных тестов и соответствовать следующим требованиям:
// 1. созданы на базе #include "common/test/include/Test.h", например "common/utils/test/TestJson.(h|cpp)"
// 2. тесты должны быть автоматическими - то есть пользователь не должен ничего нажимать или сличать глазами. Только запуск и потом результат.
#if 0
bool TestUtils::testQueue1()
{
	const char max = 16;
	Queue<StringBuilder> queue(max);

	// Формируем массив строк с нарастающей длинной
	for (char i = 0; i < max; i++) {
		StringBuilder *s = new StringBuilder(max+1);
		if (!s->getString()) {
			uart->sendln("NULL");
			return false;
		}
		for (char t = 0; t < i; t++)
			s->add(t);
		queue.push(s);
	}

	for (char i = 0; i < max; i++) {
		StringBuilder *s = queue.pop();
		if (s->getLen() != i) {
			StringBuilder *txt = new StringBuilder();
			*txt << "Error in '" << (int)i << "' value";
			uart->sendln(txt->getString());
			return false;
		}
		delete s;
	}

	StringBuilder *s = queue.pop();
	if (s != NULL) {
		uart->sendln("Get value in empty queue!");
		return false;
	}

 return true;
}

void TestUtils::testQueue(int repetition)
{
	uart->sendln("\r\n--- Start Queue test ---");

	for (int i = 0; i < repetition; i++) {
//		wdt_reset();
		if (!testQueue1()) {
			StringBuilder txt;
			txt << "repetition: " << i;
			uart->sendln(txt.getString());
		}
	}


	//
	Queue<StringBuilder> queue(8);
	StringBuilder *s = queue.pop();
	if (s != NULL) {
		uart->sendln("Get value in empty queue (after create)!");
		uart->sendln(error);
	}

	uart->sendln(ok);
}

void Tests::testStringFinder() {
	uart->sendln("\r\nStart StringFinder");
	StringFinder finder("12", "56");
	const char *mas = "012345678";
	const char *ok_str = "34";

	for (uint8_t i = 0; i < strlen(mas); i++)
		finder.read(mas[i]);

	if (finder.isCompleted()) {
		StringBuilder *result = finder.getBetween();
		if (strcmp(result->getString(), ok_str) == 0) {
			uart->sendln(ok);
			return;
		} else uart->sendln(result->getString());
	} else uart->sendln("Matcher not Completed");

	uart->sendln(error);
}

void Tests::testAvg()
{
  uart->sendln("\r\nStart test FastAverage");

  const uint16_t size = 16;
  const uint16_t div = 8;
  const uint16_t mul = 128;
  FastAverage<uint16_t> avg1(size);
  FastAverage<uint16_t> avg2(size);

  uint16_t v1 = 0;
  for (uint16_t i = 0; i < size/div; i++)
  {
    v1 += i;
    avg1.add(i);
  }
  v1 /= (size/div);

  uint16_t v2 = -10;

  for (uint16_t i = 0; i < size*mul; i++)
  {
    avg2.add(v2);
  }


  if (avg1.getAverage() != v1 || avg1.getCount() != size/div) {
    StringBuilder str;
    str << "avg1 " <<  (const int)avg1.getAverage() << " != " << (const int)v1 << " || " << (const int)avg1.getCount() << " != " << (const int)size/div;
    uart->send(str);
    uart->sendln(error);
    return;
  }


  if (avg2.getAverage() != v2 || avg2.getCount() != size*mul) {
    StringBuilder str;
    str << "avg2 " << (const int)avg2.getAverage() << " != " << (const int)v2 << " || " << (const int)avg2.getCount() << " != " << (const int)size*mul;
    uart->send(str);
    uart->sendln(error);
    return;
  }

  int v3 = 511;
  avg2.clear();
  avg2.add(v3);
  if (avg2.getAverage() != v3 || avg2.getCount() != 1) {
    StringBuilder str;
    str << "avg2 " << (const int)avg2.getAverage() << " != " << (const int)v3 << " || " << (const int)avg2.getCount() << " != 1";
    uart->send(str);
    uart->sendln(error);
    return;
  }

    uart->sendln(ok);
}
#else
#endif

class Sha1Test : public TestSet {
public:
	Sha1Test();
	bool test();
};

TEST_SET_REGISTER(Sha1Test);

Sha1Test::Sha1Test() {
	TEST_CASE_REGISTER(Sha1Test, test);
}

bool Sha1Test::test() {
	const char *in = "Hello World!!!";

	struct sha1nfo s;
	sha1_init(&s);
	sha1_write(&s, in, strlen(in));
	uint8_t *result = sha1_result(&s);

	uint16_t hexResultSize = 41;
	char hexResult[hexResultSize];
	convertBytesToHex(result, hexResult, hexResultSize);
	TEST_STRING_EQUAL("0aa941b04274ae04dc5a9bd214f7d5214f36e6de", hexResult);

	return true;
}
