#include "mdb/slave/MdbSlavePacketReceiver.h"
#include "mdb/MdbProtocolCashless.h"
#include "uart/include/TestUart.h"
#include "utils/include/StringBuilder.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

using namespace Mdb::Cashless;

class TestMdbSlaveReceiver : public MdbSlave::Receiver {
public:
	TestMdbSlaveReceiver(StringBuilder *str) : str(str) {}
	void recvAddress() { *str << "<recvAddress>"; }
	void recvSubcommand() { *str << "<recvSubcommand>"; }
	void recvRequest(uint16_t len) { *str << "<recvRequest=" <<  len << ">"; }
	void recvConfirm() { *str << "<recvConfirm>"; }

private:
	StringBuilder *str;
};


class TestMdbSlavePacketReceiverObserver : public MdbSlave::PacketObserver {
public:
	TestMdbSlavePacketReceiverObserver(StringBuilder *str) : str(str) {}
	void recvRequestPacket(const uint16_t packetType, const uint8_t *data, uint16_t dataLen) {
		*str << "<recvRequestPacket=";
		str->addHex(packetType >> 8);
		str->addHex(packetType);
		*str << ",data=";
		for(uint16_t i = 0; i < dataLen; i++) {
			str->addHex(data[i]);
		}
		*str << ">";
	}
	void recvUnsupportedPacket(const uint16_t packetType) {
		*str << "<recvUnsupportedPacket=" << packetType << ">";
	}

private:
	StringBuilder *str;
};

class MdbSlavePacketReceiverTest : public TestSet {
public:
	MdbSlavePacketReceiverTest();
	bool test();
};

TEST_SET_REGISTER(MdbSlavePacketReceiverTest);

MdbSlavePacketReceiverTest::MdbSlavePacketReceiverTest() {
	TEST_CASE_REGISTER(MdbSlavePacketReceiverTest, test);
}

bool MdbSlavePacketReceiverTest::test() {
	StringBuilder str;
	TestMdbSlaveReceiver testReceiver(&str);
	TestMdbSlavePacketReceiverObserver observer(&str);
	static MdbSlavePacketReceiver::Packet packets[] = {
		{ Command_Reset, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Setup, Subcommand_SetupConfig, sizeof(SetupConfigRequest), sizeof(SetupConfigRequest) },
		{ Command_Setup, Subcommand_SetupPrices, sizeof(SetupPricesL1Request), sizeof(SetupPricesL3Request) },
		{ Command_Poll, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_VendRequest, sizeof(VendRequestRequest), sizeof(VendRequestRequestL3) },
		{ Command_Vend, Subcommand_VendCancel, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_VendSuccess, sizeof(VendSuccessRequest), sizeof(VendSuccessRequest) },
		{ Command_Vend, Subcommand_VendFailure, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_SessionComplete, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_CashSale, sizeof(VendCashSaleRequest), sizeof(VendCashSaleRequestEC) },
		{ Command_Reader, Subcommand_ReaderEnable, 0xFF, 0xFF },
		{ Command_Reader, Subcommand_ReaderDisable, 0xFF, 0xFF },
		{ Command_Reader, Subcommand_ReaderCancel, 0xFF, 0xFF },
		{ Command_Revalue, Subcommand_RevalueRequest, sizeof(RevalueRequestL2), sizeof(RevalueRequestEC) },
		{ Command_Revalue, Subcommand_RevalueLimitRequest, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionRequestId, sizeof(ExpansionRequestIdRequest), sizeof(ExpansionRequestIdRequest) },
		{ Command_Expansion, Subcommand_ExpansionEnableOptions, sizeof(ExpansionEnableOptionsRequest), sizeof(ExpansionEnableOptionsRequest) },
	};
	MdbSlavePacketReceiver receiver(Mdb::Device_CashlessDevice1, &observer, packets, sizeof(packets)/sizeof(packets[0]));
	receiver.init(&testReceiver);

	//110100000000
	receiver.recvCommand(Command_Setup);
	TEST_STRING_EQUAL("<recvSubcommand>", str.getString());
	str.clear();

	receiver.recvSubcommand(Subcommand_SetupPrices);
	TEST_STRING_EQUAL("<recvRequest=6>", str.getString());
	str.clear();

	uint8_t r1[] = { 0x11, 0x01, 0xFF, 0xFF, 0x00, 0x00 };
	receiver.recvRequest(r1, sizeof(r1));
	TEST_STRING_EQUAL("<recvRequestPacket=0101,data=1101FFFF0000>", str.getString());
	str.clear();

	//1101FFFFFFFF000000001630
	receiver.setMode(Mdb::Mode_Expanded);
	receiver.recvCommand(Command_Setup);
	TEST_STRING_EQUAL("<recvSubcommand>", str.getString());
	str.clear();

	receiver.recvSubcommand(Subcommand_SetupPrices);
	TEST_STRING_EQUAL("<recvRequest=12>", str.getString());
	str.clear();

	uint8_t r2[] = { 0x11, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x16, 0x30 };
	receiver.recvRequest(r2, sizeof(r2));
	TEST_STRING_EQUAL("<recvRequestPacket=0101,data=1101FFFFFFFF000000001630>", str.getString());
	str.clear();
	return true;
}
