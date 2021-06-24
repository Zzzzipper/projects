#include "QTicker.h"

#include "common/logger/include/Logger.h"

#include <QDebug>

#define TICK_SIZE 20 // значения тика менее 20 милисекунд таймер QT не может соблюдать

static QTicker *m_instance = NULL;

QTicker *QTicker::get() {
	if(m_instance == NULL) {
		m_instance = new QTicker();
	}
	return m_instance;
}

QTicker::QTicker() : m_consumer(NULL) {
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
	m_timer.setTimerType(Qt::PreciseTimer);
	m_timer.setSingleShot(false);
}

void QTicker::tick() {
	if(m_consumer == NULL) {
		LOG_ERROR(LOG_TIMER, "Consumer are not register");
		return;
	}
	m_consumer->tick(TICK_SIZE);
	m_consumer->execute();
}

void QTicker::registerConsumer(TimerEngine *consumer) {
	if(m_consumer != NULL) {
		LOG_WARN(LOG_TIMER, "Consumer already register");
	}
	m_consumer = consumer;
	m_timer.start(TICK_SIZE);
}
