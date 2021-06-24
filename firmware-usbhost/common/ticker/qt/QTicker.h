#ifndef LIB_FISCAL_QTICKER_H
#define LIB_FISCAL_QTICKER_H

#include "common/timer/include/TimerEngine.h"
#include "common/ticker/include/Ticker.h"

#include <QObject>
#include <QTimer>

class QTicker : public QObject {
	Q_OBJECT
public:
	static QTicker *get();
	void registerConsumer(TimerEngine *consumer);
private slots:
	void tick();
private:
	QTicker();
	virtual ~QTicker() {}
	TimerEngine *m_consumer;
	QTimer m_timer;
};

#endif

