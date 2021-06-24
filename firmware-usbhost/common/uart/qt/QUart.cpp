#include "QUart.h"

#include "common/logger/include/Logger.h"

#include <stdexcept>
#include <QDebug>
#include <QCoreApplication>

QT_USE_NAMESPACE

QUart::QUart() :
	recvHandler(NULL),
	transmitHandler(NULL)
{
	connect(&serial, SIGNAL(readyRead()), SLOT(handleReadyRead()));
	connect(&serial, SIGNAL(bytesWritten(qint64)), SLOT(handleBytesWritten(qint64)));
	connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));
	codec = QTextCodec::codecForName("Windows-1251");
}

bool QUart::open(const char* serialPortName, BaudRate baudRate) {
	LOG_INFO(LOG_UART, "open " << serialPortName << "," << baudRate);
	if(serial.isOpen() == true) {
		serial.close();
	}

	serial.setPortName(serialPortName);
	serial.setBaudRate(baudRate);
	serial.setDataBits(QSerialPort::Data8);
	serial.setParity(QSerialPort::NoParity);
	serial.setStopBits(QSerialPort::OneStop);
//	serial.setFlowControl(QSerialPort::SoftwareControl);

	if(serial.open(QIODevice::ReadWrite) == false) {
		QByteArray port = codec->fromUnicode(serial.portName());
		QByteArray error = codec->fromUnicode(serial.errorString());
		LOG_ERROR(LOG_UART, "Open " << port.data() << " failed: " << error.data());
		return false;
	}

	return true;
}

void QUart::send(uint8_t b) {
	LOG_TRACE(LOG_UART, "send " << b);
	sendBuf.append(b);
	qint64 bytesWritten = serial.write(sendBuf);
	if(bytesWritten == -1) {
		QByteArray port = codec->fromUnicode(serial.portName());
		QByteArray error = codec->fromUnicode(serial.errorString());
		LOG_ERROR(LOG_UART, "Write to " << port.data() << " failed: " << error.data());
	} else if (bytesWritten != sendBuf.size()) {
		sendBuf.remove(0, bytesWritten);
	} else {
		sendBuf.clear();
	}
}

void QUart::sendAsync(uint8_t b) {
	send(b);
}

uint8_t QUart::receive() {
	if(recvBuf.isEmpty() == true) {
		QByteArray port = codec->fromUnicode(serial.portName());
		LOG_ERROR(LOG_UART, "No data in " << port.data());
		return 0;
	}
	uint8_t data = recvBuf[0];
	recvBuf.remove(0,1);
	return data;
}

bool QUart::isEmptyReceiveBuffer() {
	return recvBuf.isEmpty();
}

bool QUart::isFullTransmitBuffer() {
	return false;
}

void QUart::setReceiveHandler(UartReceiveHandler *handler) {
	recvHandler = handler;
}

void QUart::setTransmitHandler(UartTransmitHandler *handler) {
	transmitHandler = handler;
}

void QUart::execute() {
	return;
}

void QUart::handleReadyRead() {
	LOG_TRACE(LOG_UART, "handleReadyRead");
	recvBuf.append(serial.readAll());
	if(recvHandler != NULL) {
		recvHandler->handle();
	}
}

void QUart::handleError(QSerialPort::SerialPortError serialPortError) {
	LOG_TRACE(LOG_UART, "handleError" << serialPortError);
	if(serialPortError == QSerialPort::ReadError) {
		QByteArray port = codec->fromUnicode(serial.portName());
		QByteArray error = codec->fromUnicode(serial.errorString());
		LOG_ERROR(LOG_UART, "Port " << port.data() << " fatal error: " << error.data());
	}
}

void QUart::handleBytesWritten(qint64 ) {
	LOG_TRACE(LOG_UART, "handleBytesWritten");
	if(sendBuf.isEmpty() == true) {
		return;
	}

	qint64 bytesWritten = serial.write(sendBuf);
	if(bytesWritten == -1) {
		QByteArray port = codec->fromUnicode(serial.portName());
		QByteArray error = codec->fromUnicode(serial.errorString());
		LOG_ERROR(LOG_UART, "Write to " << port.data() << " failed: " << error.data());
	} else if (bytesWritten != sendBuf.size()) {
		sendBuf.remove(0, bytesWritten);
	} else {
		sendBuf.clear();
	}
}

void QUart::close() {
	serial.close();
}
