 #include "TestDataParser.h"

TestDataParser::TestDataParser() :
	hex(true),
	startResult(Result_Ok),
	procDataResult(Result_Ok),
	completeResult(Result_Ok)
{
}

TestDataParser::TestDataParser(uint16_t maxSize, bool hex) :
	hex(hex),
	data(maxSize, maxSize),
	startResult(Result_Ok),
	procDataResult(Result_Ok),
	completeResult(Result_Ok)
{
}

TestDataParser::~TestDataParser() {}

void TestDataParser::setStartResult(Result result) {
	this->startResult = result;
}

void TestDataParser::setProcDataResult(Result result) {
	this->procDataResult = result;
}

void TestDataParser::setCompleteResult(Result result) {
	this->completeResult = result;
}

void TestDataParser::clearData() {
	data.clear();
}

const char *TestDataParser::getData() {
	return data.getString();
}

Dex::DataParser::Result TestDataParser::start(uint32_t dataSize) {
	StringBuilder str;
	str << "<start=" << dataSize << ">";
	data.set(str.getString());
	return startResult;
}

Dex::DataParser::Result TestDataParser::procData(const uint8_t *data, const uint16_t len) {
	if(hex == true) {
		for(uint16_t i = 0; i < len; i++) {
			this->data.addHex(data[i]);
		}
	} else {
		for(uint16_t i = 0; i < len; i++) {
			this->data.add(data[i]);
		}
	}
	return procDataResult;
}

Dex::DataParser::Result TestDataParser::complete() {
	data.addStr("<complete>");
	return completeResult;
}

void TestDataParser::error() {
	data.addStr("<error>");
}
