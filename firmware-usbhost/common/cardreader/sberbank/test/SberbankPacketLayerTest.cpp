#include "cardreader/sberbank/SberbankPacketLayer.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Hex.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestSberbankFrameLayer : public Sberbank::FrameLayerInterface {
public:
	TestSberbankFrameLayer(StringBuilder *result) : result(result), observer(NULL) {}
	virtual void setObserver(Sberbank::FrameLayerObserver *observer) { this->observer = observer; }
	virtual void reset() {
		*result << "<FL::reset>";
	}
	virtual bool sendPacket(Buffer *data) {
		*result << "<FL::sendPacket=" << data->getLen() << ",data=";
		for(uint16_t i = 0; i < data->getLen(); i++) {
			result->addHex((*data)[i]);
		}
		*result << ">";
		return true;
	}
	virtual bool sendControl(uint8_t control) {
		*result << "<FL::sendControl=" << control << ">";
		return true;
	}
	bool addRecvData(const char *hex) {
		Buffer buf(SBERBANK_FRAME_SIZE);
		hexToData(hex, &buf);
		observer->procPacket(buf.getData(), buf.getLen());
		return true;
	}

private:
	StringBuilder *result;
	Sberbank::FrameLayerObserver *observer;
};

class TestSberbankPacketLayerObserver : public Sberbank::PacketLayerObserver {
public:
	TestSberbankPacketLayerObserver(StringBuilder *result) : result(result) {}
	virtual void procPacket(const uint8_t *data, const uint16_t dataLen) {
		*result << "<event=RecvData,len=" << dataLen << ",data=";
		for(uint16_t i = 0; i < dataLen; i++) {
			result->addHex(data[i]);
		}
		*result << ">";
	}
	virtual void procControl(uint8_t control) {
		*result << "<control=" << control << ">";
	}
	virtual void procError(Error error) {
		switch(error) {
//		case TaskLayerObserver::Error_OK: *str << "<event=OK>"; return;
//		case TaskLayerObserver::Error_ConnectFailed: *str << "<event=ConnectFailed>"; return;
//		case TaskLayerObserver::Error_RemoteClose: *str << "<event=RemoteClose>"; return;
//		case TaskLayerObserver::Error_SendFailed: *str << "<event=SendFailed>"; return;
//		case TaskLayerObserver::Error_RecvFailed: *str << "<event=RecvFailed>"; return;
//		case TaskLayerObserver::Error_RecvTimeout: *str << "<event=RecvTimeout>"; return;
		default: *result << "<event=" << error << ">";
		}
	}

private:
	StringBuilder *result;
};

class SberbankPacketLayerTest : public TestSet {
public:
	SberbankPacketLayerTest();
	bool init();
	void cleanup();
	bool test();
	bool testBrokenChain();
	bool testSendBigData();

private:
	StringBuilder *result;
	TimerEngine *timerEngine;
	TestSberbankFrameLayer *frameLayer;
	TestSberbankPacketLayerObserver *observer;
	Sberbank::PacketLayer *layer;
};

TEST_SET_REGISTER(SberbankPacketLayerTest);

SberbankPacketLayerTest::SberbankPacketLayerTest() {
	TEST_CASE_REGISTER(SberbankPacketLayerTest, test);
	TEST_CASE_REGISTER(SberbankPacketLayerTest, testBrokenChain);
	TEST_CASE_REGISTER(SberbankPacketLayerTest, testSendBigData);
}

bool SberbankPacketLayerTest::init() {
	this->result = new StringBuilder(2048, 2048);
	this->timerEngine = new TimerEngine();
	this->frameLayer = new TestSberbankFrameLayer(result);
	this->observer = new TestSberbankPacketLayerObserver(result);
	this->layer = new Sberbank::PacketLayer(this->timerEngine, this->frameLayer);
	this->layer->setObserver(observer);
	return true;
}

void SberbankPacketLayerTest::cleanup() {
	delete this->layer;
	delete this->observer;
	delete this->frameLayer;
	delete this->timerEngine;
	delete this->result;
}

bool SberbankPacketLayerTest::test() {
	layer->reset();

	// send
	Buffer data1(256);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("c00c001d440a00cd00000000000000ffffffff", &data1));
	layer->sendPacket(&data1);
	TEST_STRING_EQUAL("<FL::reset><FL::sendPacket=23,data=0013C00C001D440A00CD00000000000000FFFFFFFF661D>", result->getString());
	result->clear();

	// recv frame1
	TEST_NUMBER_EQUAL(true, frameLayer->addRecvData(
		"80b400bc00f66e07800000323730333335003833"
		"3239353131353735343600303030310034323736"
		"36332a2a2a2a2a2a323931350000000030322f32"
		"30008e848e819085488e3a000000000000000000"
		"000000000000000000000000000085f03301460a"
		"0200013230303233313431005669736100000000"
		"0000000000000000363331303030303030303436"
		"000000008171e79003bfc422a73796f0ee4e0c5b"
		"a05d4d56d2634db744cab644740e69c9c4ddaa6b"
		"5f828a3a"));
	TEST_STRING_EQUAL("<FL::sendControl=6>", result->getString());
	result->clear();

	// recv frame2
	TEST_NUMBER_EQUAL(true, frameLayer->addRecvData(
		"010fc54f4a2097b251e70627a1357bc803c719"));
	TEST_STRING_EQUAL("<FL::sendControl=4><event=RecvData,len=195,data="
		"00BC00F66E078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2A2A2A2A2A2A323931350000000030322F323000"
		"8E848E819085488E3A0000000000000000000000"
		"00000000000000000000000085F03301460A0200"
		"0132303032333134310056697361000000000000"
		"0000000000003633313030303030303034360000"
		"00008171E79003BFC422A73796F0EE4E0C5BA05D"
		"4D56D2634DB744CAB644740E69C9C4DDAA6B5F82"
		"C54F4A2097B251E70627A1357BC803>",
		result->getString());
	result->clear();
	return true;
}

bool SberbankPacketLayerTest::testBrokenChain() {
	layer->reset();

	// send
	Buffer data1(256);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("c00c001d440a00cd00000000000000ffffffff", &data1));
	layer->sendPacket(&data1);
	TEST_STRING_EQUAL("<FL::reset><FL::sendPacket=23,data=0013C00C001D440A00CD00000000000000FFFFFFFF661D>", result->getString());
	result->clear();

	// recv frame1
	TEST_NUMBER_EQUAL(true, frameLayer->addRecvData(
		"80b400bc00f66e07800000323730333335003833"
		"3239353131353735343600303030310034323736"
		"36332a2a2a2a2a2a323931350000000030322f32"
		"30008e848e819085488e3a000000000000000000"
		"000000000000000000000000000085f03301460a"
		"0200013230303233313431005669736100000000"
		"0000000000000000363331303030303030303436"
		"000000008171e79003bfc422a73796f0ee4e0c5b"
		"a05d4d56d2634db744cab644740e69c9c4ddaa6b"
		"5f828a3a"));
	TEST_STRING_EQUAL("<FL::sendControl=6>", result->getString());
	result->clear();

	// wait frame
	timerEngine->tick(SBERBANK_FRAME_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	// new frame
	TEST_NUMBER_EQUAL(true, frameLayer->addRecvData(
		"000B0004001D440A8009200000973D"));
	TEST_STRING_EQUAL("<FL::sendControl=4><event=RecvData,len=11,data="
		"0004001D440A8009200000>",
		result->getString());
	result->clear();
	return true;
}

bool SberbankPacketLayerTest::testSendBigData() {
	layer->reset();
	TEST_STRING_EQUAL("<FL::reset>", result->getString());
	result->clear();

	// send
	Buffer data1(2048);
	TEST_NUMBER_NOT_EQUAL(0, hexToData(
		"0009022200008002190004020704005300424753"
		"014d51ab0044e350020000001e00690000000201"
		"343c47000209b50000fe7800000000ff06820183"
		"df0d0101df1682017a2200000022049999122221"
		"0000272099990130000000309999990630880000"
		"3094999915309600003102999915311200003120"
		"9999153158000031599999153337000033499999"
		"1534000000349999990535280000358999991536"
		"0000003699999906370000003799999905380000"
		"0039999999064000000049999999034276800042"
		"7689990450000000509999990251000000559999"
		"9901560000005999999902600000006999999902"
		"6011000060110999066011200060114999066011"
		"7400601174990660117700601177990660118600"
		"6011999906605461006054629907620000006299"
		"99990e6440000065999999067005230070052399"
		"3178255510782555193160598200605982993178"
		"2599007825999931782601007826019931701342"
		"0070134209317084271170842711317825460078"
		"2546993081000000819999990e90000000905099"
		"990e90510000905199991290520000909999990e"
		"91110000911199990e94000000998999990e9990"
		"00009999999908ff06820183df0d0102df168201"
		"7a2202200022022099ff3751220037512499ff40"
		"43240040432499fe4080730040807499fe408240"
		"0040824199fe4083440040834599fe4096530040"
		"965399fe41034000", &data1));
	layer->sendPacket(&data1);
	TEST_STRING_EQUAL(
		"<FL::sendPacket=184,data="
		"80B4000902220000800219000402070400530042"
		"4753014D51AB0044E350020000001E0069000000"
		"0201343C47000209B50000FE7800000000FF0682"
		"0183DF0D0101DF1682017A220000002204999912"
		"2221000027209999013000000030999999063088"
		"0000309499991530960000310299991531120000"
		"3120999915315800003159999915333700003349"
		"9999153400000034999999053528000035899999"
		"1536000000369999990637000000379999990538"
		"000073F2>", result->getString());
	result->clear();

	layer->procControl(Sberbank::Control_ACK);
	TEST_STRING_EQUAL(
		"<FL::sendPacket=184,data="
		"81B4003999999906400000004999999903427680"
		"0042768999045000000050999999025100000055"
		"9999990156000000599999990260000000699999"
		"9902601100006011099906601120006011499906"
		"6011740060117499066011770060117799066011"
		"8600601199990660546100605462990762000000"
		"629999990E644000006599999906700523007005"
		"2399317825551078255519316059820060598299"
		"3178259900782599993178260100782601993170"
		"13420C69>", result->getString());
	result->clear();

	layer->procControl(Sberbank::Control_BEL);
	TEST_STRING_EQUAL(
		"<FL::sendPacket=172,data="
		"02A8007013420931708427117084271131782546"
		"00782546993081000000819999990E9000000090"
		"5099990E90510000905199991290520000909999"
		"990E91110000911199990E94000000998999990E"
		"999000009999999908FF06820183DF0D0102DF16"
		"82017A2202200022022099FF3751220037512499"
		"FF4043240040432499FE4080730040807499FE40"
		"82400040824199FE4083440040834599FE409653"
		"0040965399FE41034000D109>", result->getString());
	result->clear();

	layer->procControl(Sberbank::Control_EOT);
	return true;
}
