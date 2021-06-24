#ifndef QUART_H
#define QUART_H

#include "common/uart/include/interface.h"

#include <QObject>
#include <QTextCodec>
#include <QtSerialPort>

QT_USE_NAMESPACE

class QUart : public QObject, public AbstractUart {
	Q_OBJECT
public:
	enum BaudRate {
		BaudRate1200 = 1200,
		BaudRate2400 = 2400,
		BaudRate4800 = 4800,
		BaudRate9600 = 9600,
		BaudRate14400 = 14400,
		BaudRate19200 = 19200,
		BaudRate38400 = 38400,
		BaudRate57600 = 57600,
		BaudRate115200 = 115200,
		BaudRate256000 = 256000,
	};

	QUart();
	bool open(const char* serialPortName, BaudRate baudRate);
	void send(uint8_t b);
	void sendAsync(uint8_t b);
	uint8_t receive();
	bool isEmptyReceiveBuffer();
	bool isFullTransmitBuffer();
	void setReceiveHandler(UartReceiveHandler *handler);
	void setTransmitHandler(UartTransmitHandler *handler);
	void execute();
	void close();

private slots:
	void handleBytesWritten(qint64 bytes);
	void handleReadyRead();
	void handleError(QSerialPort::SerialPortError serialPortError);

private:
	QSerialPort serial;
	QByteArray recvBuf;
	QByteArray sendBuf;
	UartReceiveHandler *recvHandler;
	UartTransmitHandler *transmitHandler;
	QTextCodec *codec;
};

#endif // UART_H
