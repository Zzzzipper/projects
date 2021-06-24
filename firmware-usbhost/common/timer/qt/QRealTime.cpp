#include "QRealTime.h"

#include <QDateTime>

bool QRealTime::setDateTime(DateTime *date) {
	(void)date;
	return false;
}

void QRealTime::getDateTime(DateTime *datetime) {
	QDateTime now = QDateTime::currentDateTime();
	datetime->year = now.date().year() - 2000;
	datetime->month = now.date().month();
	datetime->day = now.date().day();
	datetime->hour = now.time().hour();
	datetime->minute = now.time().minute();
	datetime->second = now.time().second();
}

uint32_t QRealTime::getUnixTimestamp() {
	QDateTime now = QDateTime::currentDateTime();
	return now.toSecsSinceEpoch();
}
