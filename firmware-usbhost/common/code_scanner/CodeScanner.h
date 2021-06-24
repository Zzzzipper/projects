#ifndef COMMON_CODESCANNER_H_
#define COMMON_CODESCANNER_H_

#include "mdb/MdbProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

class CodeScannerInterface {
public:
	class Observer {
	public:
		virtual ~Observer() {}
		virtual bool procCode(uint8_t *data, uint16_t dataLen) = 0;
	};

	virtual ~CodeScannerInterface() {}
	virtual void addObserver(Observer *observer) = 0;
	virtual void on() = 0;
	virtual void off() = 0;
	virtual void test() = 0;
};

class CodeScannerFake : public CodeScannerInterface {
public:
	CodeScannerFake();
	void addObserver(Observer *observer) override;
	void on() override {}
	void off() override {}
	void test() override;

private:
	List<Observer> observers;

	void procCode(uint8_t *code, uint32_t codeLen);
	void testUnitex1();
	void testNefteMag();
	void testErpOrder();
};

class CodeScanner : public CodeScannerInterface, public UartReceiveHandler {
public:
	CodeScanner(Mdb::DeviceContext *context, AbstractUart *uart, TimerEngine *timers);
	void addObserver(Observer *observer) override;
	void on() override;
	void off() override;
	void test() override;

	void handle();
	void procTimer();

private:
	Mdb::DeviceContext *context;
	TimerEngine *timers;
	Timer *timer;
	AbstractUart *uart;
	List<Observer> observers;
	Buffer data;

	void procCode(uint8_t *code, uint32_t codeLen);
};

#endif
