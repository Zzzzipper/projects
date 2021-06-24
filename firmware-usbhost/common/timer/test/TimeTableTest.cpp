#include "timer/include/TimeTable.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TimeTableTest : public TestSet {
public:
	TimeTableTest();
	bool testWeekDay();
	bool testWeekTable();
	bool testTimeInterval();
	bool testTimeTable();
	bool testStringToDTime();
};

TEST_SET_REGISTER(TimeTableTest);

TimeTableTest::TimeTableTest() {
	TEST_CASE_REGISTER(TimeTableTest, testWeekDay);
	TEST_CASE_REGISTER(TimeTableTest, testWeekTable);
	TEST_CASE_REGISTER(TimeTableTest, testTimeInterval);
	TEST_CASE_REGISTER(TimeTableTest, testTimeTable);
	TEST_CASE_REGISTER(TimeTableTest, testStringToDTime);
}

bool TimeTableTest::testWeekDay() {
	DateTime datetime1(18, 05, 14, 00, 00, 00);
	TEST_NUMBER_EQUAL(WeekDay_Monday, datetime1.getWeekDay());
	DateTime datetime2(18, 05, 15, 00, 00, 00);
	TEST_NUMBER_EQUAL(WeekDay_Tuesday, datetime2.getWeekDay());
	DateTime datetime3(18, 05, 16, 00, 00, 00);
	TEST_NUMBER_EQUAL(WeekDay_Wednesday, datetime3.getWeekDay());
	DateTime datetime4(18, 05, 17, 00, 00, 00);
	TEST_NUMBER_EQUAL(WeekDay_Thursday, datetime4.getWeekDay());
	DateTime datetime5(18, 05, 18, 00, 00, 00);
	TEST_NUMBER_EQUAL(WeekDay_Friday, datetime5.getWeekDay());
	DateTime datetime6(18, 05, 19, 00, 00, 00);
	TEST_NUMBER_EQUAL(WeekDay_Saturday, datetime6.getWeekDay());
	DateTime datetime7(18, 05, 20, 00, 00, 00);
	TEST_NUMBER_EQUAL(WeekDay_Sunday, datetime7.getWeekDay());
	return true;
}

bool TimeTableTest::testWeekTable() {
	WeekTable wt;

	wt.set(WeekDay_Sunday);
	wt.set(WeekDay_Thursday);
	TEST_NUMBER_EQUAL(0x11, wt.getValue());
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Monday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Tuesday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Wednesday));
	TEST_NUMBER_EQUAL(true, wt.check(WeekDay_Thursday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Friday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Saturday));
	TEST_NUMBER_EQUAL(true, wt.check(WeekDay_Sunday));

	wt.unset(WeekDay_Sunday);
	TEST_NUMBER_EQUAL(0x10, wt.getValue());
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Monday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Tuesday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Wednesday));
	TEST_NUMBER_EQUAL(true, wt.check(WeekDay_Thursday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Friday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Saturday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Sunday));

	wt.toggle(WeekDay_Friday);
	TEST_NUMBER_EQUAL(0x30, wt.getValue());
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Monday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Tuesday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Wednesday));
	TEST_NUMBER_EQUAL(true, wt.check(WeekDay_Thursday));
	TEST_NUMBER_EQUAL(true, wt.check(WeekDay_Friday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Saturday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Sunday));

	wt.toggle(WeekDay_Friday);
	TEST_NUMBER_EQUAL(0x10, wt.getValue());
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Monday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Tuesday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Wednesday));
	TEST_NUMBER_EQUAL(true, wt.check(WeekDay_Thursday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Friday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Saturday));
	TEST_NUMBER_EQUAL(false, wt.check(WeekDay_Sunday));
	return true;
}

bool TimeTableTest::testTimeInterval() {
	TimeInterval ti;
	ti.set(10, 15, 30, 90*60);
	TEST_NUMBER_EQUAL(10, ti.getTime()->getHour());
	TEST_NUMBER_EQUAL(15, ti.getTime()->getMinute());
	TEST_NUMBER_EQUAL(30, ti.getTime()->getSecond());
	TEST_NUMBER_EQUAL(5400, ti.getInterval());

	Time t1(10, 10, 0);
	TEST_NUMBER_EQUAL(false, ti.check(t1.getTotalSeconds()));
	Time t2(10, 30, 0);
	TEST_NUMBER_EQUAL(true, ti.check(t2.getTotalSeconds()));
	Time t3(11, 30, 0);
	TEST_NUMBER_EQUAL(true, ti.check(t3.getTotalSeconds()));
	Time t4(11, 50, 0);
	TEST_NUMBER_EQUAL(false, ti.check(t4.getTotalSeconds()));
	return true;
}

bool TimeTableTest::testTimeTable() {
	TimeTable tt;
	tt.setWeekDay(WeekDay_Monday, true);
	tt.setWeekDay(WeekDay_Tuesday, true);
	tt.setInterval(16, 0, 0, 7200);

	DateTime datetime1(18, 5, 20, 18, 19, 00);
	TEST_NUMBER_EQUAL(false, tt.check(datetime1.getTimeSeconds(), datetime1.getWeekDay()));
	DateTime datetime2(18, 5, 21, 18, 19, 00);
	TEST_NUMBER_EQUAL(false, tt.check(datetime2.getTimeSeconds(), datetime2.getWeekDay()));
	DateTime datetime3(18, 5, 21, 17, 19, 00);
	TEST_NUMBER_EQUAL(true, tt.check(datetime3.getTimeSeconds(), datetime3.getWeekDay()));
	return true;
}

bool TimeTableTest::testStringToDTime() {
	Time t1;
	stringToTime("10:20:30", &t1);
	TEST_NUMBER_EQUAL(10, t1.getHour());
	TEST_NUMBER_EQUAL(20, t1.getMinute());
	TEST_NUMBER_EQUAL(30, t1.getSecond());
	return true;
}
