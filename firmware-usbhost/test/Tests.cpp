#include "Tests.h"

#include "common/logger/include/LogTargetUart.h"
#include "common/logger/include/Logger.h"
#include "common/uart/stm32/include/uart.h"

void testMain() {
	SystemCoreClockUpdate();

	// Инициализация отладочного UART
	Uart *debugUart = Uart::get(DEBUG_UART);
	debugUart->setup(115200, Uart::Parity_None, 0);
	debugUart->setTransmitBufferSize(2048);
	debugUart->setReceiveBufferSize(512);
	Logger::get()->registerTarget(new LogTargetUart(debugUart));

	__enable_irq();

	TestEngine::get()->add("TestPlatform", new TestPlatform);
	TestEngine::get()->test();

	while(1) {
		IWDG_ReloadCounter();
		debugUart->execute();
	}
}

TEST_SET_REGISTER(TestPlatform);

TestPlatform::TestPlatform() {
	TEST_CASE_REGISTER(TestPlatform, testOperatorNew);
//	TEST_CASE_REGISTER(TestPlatform, testUart9Bit); тест требует подключения разъемов
}

static char testClassIndex = '0';

class TestClass {
public:
	TestClass() {
		strcpy(this->name, "default ");
		name[8] = testClassIndex++;
		name[9] = 0;
		TestEcho::stream() << "--> Constructor(): " << name;
	}

	TestClass(const char *name) {
		strcpy(this->name, name);
		TestEcho::stream() << "--> Constructor(name): " << name;
	}

	~TestClass() {
		TestEcho::stream() << "<-- Destructor(): " << name;
	}

	char * getName() {
		return name;
	}

private :
	char name[16];
};

/**
 * Про оператор new: http://www.amse.ru/courses/cpp2/2011_03_21.html
 */
bool TestPlatform::testOperatorNew() {
	// Создаем объект в стеке. Вызывается конструктор. Размер объекта: 20 байт.
	TestEcho::clear();
	TestClass t0("test 0");
	TEST_STRING_EQUAL("--> Constructor(name): test 0", TestEcho::getString());

	// Создаем указатель на объект. Вызывается конструктор. Размер объекта: 20 байт.
	TestEcho::clear();
	TestClass *t1 = new TestClass("test 1");
	TEST_STRING_EQUAL("--> Constructor(name): test 1", TestEcho::getString());

	// Создаем указатель на МАССИВ из 3-х объектов. Для каждого из объектов вызывается конструктор ПО УМОЛЧАНИЮ. Размер выделенной памяти: 20 * 3 + 8 = 68 байт.
	TestEcho::clear();
	TestClass *t2 = new TestClass[3];
	TEST_STRING_EQUAL("--> Constructor(): default 0--> Constructor(): default 1--> Constructor(): default 2", TestEcho::getString());

	// Создаем массив из 3-х указателей на объекты TestClass. Сами объекты не создаются. Размер выделенной памяти (для АРМ): 3 * 4 = 12 байт.
	TestEcho::clear();
	TestClass **tt3 = new TestClass*[3];
	TEST_STRING_EQUAL("", TestEcho::getString());

	// Удаляем объект t1. Вызывается деструктор.
	TestEcho::clear();
	delete t1;
	TEST_STRING_EQUAL("<-- Destructor(): test 1", TestEcho::getString());

	// Удаляем массив объектов t2. Вызывается деструктор для каждого объекта.
	TestEcho::clear();
	delete []t2;
	TEST_STRING_EQUAL("<-- Destructor(): default 2<-- Destructor(): default 1<-- Destructor(): default 0", TestEcho::getString());

	// Удаляем массив указателей t3. Объекты, созданные внутри массива, должны быть удалены ЗАРАНЕЕ.
	TestEcho::clear();
	delete []tt3;
	TEST_STRING_EQUAL("", TestEcho::getString());

	return true;
}

class TestTransmitHandler: public UartTransmitHandler {
public:
	TestTransmitHandler() {
		transmitBufferIsEmpty = false;
	}

	virtual void emptyTransmitBuffer() {
		transmitBufferIsEmpty = true;
	}
	
	bool isEmptyTransmitBuffer() {
		return transmitBufferIsEmpty;
	}

private:
	volatile bool transmitBufferIsEmpty;
};

class TestReceiveHandler : public UartReceiveHandler {
public:
	TestReceiveHandler(uint16_t len) : UartReceiveHandler(len), received(false) {}
	bool isReceived() {
		return received;
	}

	void handle() {
		received = true;
	}

private:
	volatile bool received;
};

/*
 * Start test Uart 9 bit
 * Interconnect pin Tx3 and Rx1 for this testing ...
 * Тест виснет, если не подключить контакты.
 */
bool TestPlatform::testUart9Bit() {
	Uart *u9_sender = Uart::get(UART_3);
	Uart *u9_receiver = Uart::get(UART_1);

	u9_sender->setup(57600, Uart::Parity_None, 0, Uart::Mode_9Bit);
	u9_receiver->setup(57600, Uart::Parity_None, 0, Uart::Mode_9Bit);
	
	u9_sender->setTransmitBufferSize(64);	
	u9_receiver->setReceiveBufferSize(64);
	u9_sender->clear();
	u9_receiver->clear();
	
	const uint8_t s = 22;
	TestReceiveHandler receiveHandler(s);
	u9_receiver->setReceiveHandler(&receiveHandler);
	
	TestTransmitHandler transmitHandler;
	u9_sender->setTransmitHandler(&transmitHandler);
	
	uint8_t sendBytes[s] = {0x01, '1', 0x01, '2', 0x01, '3', 0x01, '4', 0x00, '5', 0x00, '6', 0x00, '7', 0x00, '8', 0x00, '9', 0x00, 'A', 0x00, 'B'};
	uint8_t receiveBytes[s];
	
	for(int i = 0; i < s; i++) {
		u9_sender->send(sendBytes[i]);
	}
	
	while(!transmitHandler.isEmptyTransmitBuffer()) {
		u9_sender->execute();
		u9_receiver->execute();
	}
	u9_sender->clear();
	
	while(!receiveHandler.isReceived()) {
		u9_sender->execute();
		u9_receiver->execute();		
	}
	
	StringBuilder txt("Received: ");
	uint8_t err = 0;
	for(uint8_t i = 0; i < s; i++) {
		receiveBytes[i] = u9_receiver->receive();
		txt << receiveBytes[i] << " ";
		if(sendBytes[i] != receiveBytes[i]) {
			err++;
		}		
	}	

	TEST_STRING_EQUAL("Received: xz", txt.getString());
	return true;
}
