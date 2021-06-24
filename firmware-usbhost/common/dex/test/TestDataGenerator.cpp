#include "TestDataGenerator.h"

TestDataGenerator::TestDataGenerator() : state(State_Idle) {
}

TestDataGenerator::~TestDataGenerator() {
}

void TestDataGenerator::reset() {
	state = State_Data0;
	data.set("11111");
}

void TestDataGenerator::next() {
    switch(state) {
		case State_Data0: {
			data.set("2222233333");
			state = State_Data1;
			break;
		}
		case State_Data1: {
			data.set("444445555566666");
			state = State_Data2;
			break;
		}
		case State_Data2: {
			state = State_Idle;
			break;
		}
		default:;
    }
}

bool TestDataGenerator::isLast() {
	return state == State_Data2;
}

const void *TestDataGenerator::getData() {
	return data.getString();
}

uint16_t TestDataGenerator::getLen( ){
	return data.getLen();
}
