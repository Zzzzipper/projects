#include "test/include/Test.h"
#include "timer/include/RealTime.h"
#include "logger/include/Logger.h"

#include <QDateTime>

class DateTimeTest : public TestSet {
public:
	DateTimeTest();
	bool test();
	bool testStringToDateTime();
	bool testFiscal2DateTime();
	bool testDateTime2Fiscal();
};

TEST_SET_REGISTER(DateTimeTest);

DateTimeTest::DateTimeTest() {
	TEST_CASE_REGISTER(DateTimeTest, test);
	TEST_CASE_REGISTER(DateTimeTest, testStringToDateTime);
	TEST_CASE_REGISTER(DateTimeTest, testFiscal2DateTime);
	TEST_CASE_REGISTER(DateTimeTest, testDateTime2Fiscal);
}

bool DateTimeTest::test() {
	// Проверка исходных данных с помощью классов QT
	QDateTime qd0(QDate(2001, 1, 1), QTime(0, 0, 0), Qt::UTC);
	QDateTime qd2(QDate(2010, 6, 15), QTime(10, 20, 30), Qt::UTC);
	QDateTime qd3(QDate(2012, 3, 4), QTime(20, 30, 40), Qt::UTC);
	QDateTime qd4(QDate(2004, 6, 28), QTime(20, 50, 50), Qt::UTC);
	TEST_NUMBER_EQUAL(3452, (int32_t)qd0.daysTo(qd2));
	TEST_NUMBER_EQUAL(298290030, (int32_t)qd0.secsTo(qd2));
	TEST_NUMBER_EQUAL(54295810, (int32_t)qd2.secsTo(qd3));
	TEST_STRING_EQUAL("06/03/19 07:01:00", qd4.addSecs(54295810).toString("yy/MM/dd hh:mm:ss").toStdString().c_str());

	DateTime d1;
	d1.year = 10;
	d1.month = 6;
	d1.day = 15;
	d1.hour = 10;
	d1.minute = 20;
	d1.second = 30;
	TEST_NUMBER_EQUAL(151, d1.getMonthDays());
	TEST_NUMBER_EQUAL(3287, d1.getYearDays());
	TEST_NUMBER_EQUAL(3452, d1.getTotalDays());
	int32_t totalSeconds = 298290030;
	TEST_NUMBER_EQUAL(totalSeconds, d1.getTotalSeconds());

	DateTime d2;
	d2.set(totalSeconds);
	TEST_NUMBER_EQUAL(10, d2.hour);
	TEST_NUMBER_EQUAL(20, d2.minute);
	TEST_NUMBER_EQUAL(30, d2.second);
	TEST_NUMBER_EQUAL(10, d2.year);
	TEST_NUMBER_EQUAL(6, d2.month);
	TEST_NUMBER_EQUAL(15, d2.day);

	DateTime d3;
	d3.year = 12;
	d3.month = 3;
	d3.day = 4;
	d3.hour = 20;
	d3.minute = 30;
	d3.second = 40;
	int32_t seconds1 = d1.secondsTo(&d3);
	TEST_NUMBER_EQUAL(54295810, seconds1);

	DateTime d4;
	d4.year = 4;
	d4.month = 6;
	d4.day = 28;
	d4.hour = 20;
	d4.minute = 50;
	d4.second = 50;
	TEST_NUMBER_EQUAL(true, d4.addSeconds(seconds1));
	TEST_NUMBER_EQUAL(7, d4.hour);
	TEST_NUMBER_EQUAL(1, d4.minute);
	TEST_NUMBER_EQUAL(0, d4.second);
	TEST_NUMBER_EQUAL(6, d4.year);
	TEST_NUMBER_EQUAL(3, d4.month);
	TEST_NUMBER_EQUAL(19, d4.day);

	return true;
}

bool DateTimeTest::testStringToDateTime() {
	DateTime d1;
	stringToDateTime("2010-06-15 10:20:30", &d1);
	TEST_NUMBER_EQUAL(10, d1.hour);
	TEST_NUMBER_EQUAL(20, d1.minute);
	TEST_NUMBER_EQUAL(30, d1.second);
	TEST_NUMBER_EQUAL(10, d1.year);
	TEST_NUMBER_EQUAL(6, d1.month);
	TEST_NUMBER_EQUAL(15, d1.day);
	return true;
}

bool DateTimeTest::testFiscal2DateTime() {
	const char f1[] = "20100615T1020";
	DateTime d1;
	TEST_NUMBER_EQUAL(13, fiscal2datetime(f1, sizeof(f1), &d1));
	TEST_NUMBER_EQUAL(10, d1.year);
	TEST_NUMBER_EQUAL(6, d1.month);
	TEST_NUMBER_EQUAL(15, d1.day);
	TEST_NUMBER_EQUAL(10, d1.hour);
	TEST_NUMBER_EQUAL(20, d1.minute);
	TEST_NUMBER_EQUAL(0, d1.second);

	const char f2[] = "20100615T10";
	DateTime d2;
	TEST_NUMBER_EQUAL(0, fiscal2datetime(f2, sizeof(f2), &d2));
	return true;
}

bool DateTimeTest::testDateTime2Fiscal() {
	DateTime d1;
	d1.year = 10;
	d1.month = 6;
	d1.day = 15;
	d1.hour = 10;
	d1.minute = 20;
	d1.second = 15;
	StringBuilder s1;
	datetime2fiscal(&d1, &s1);
	TEST_STRING_EQUAL("20100615T1020", s1.getString());
	return true;
}
