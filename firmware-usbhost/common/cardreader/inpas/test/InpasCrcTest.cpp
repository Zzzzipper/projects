#include "common/cardreader/inpas/InpasProtocol.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class InpasCrcTest : public TestSet {
public:
	InpasCrcTest();
	bool test();
	bool testTlv();
	bool testSoh();
	bool testTlvPacketMaker();
	bool testUsbPacketCrc();
	bool testUnitexQrCodeCrc();
};

TEST_SET_REGISTER(InpasCrcTest);

InpasCrcTest::InpasCrcTest() {
	TEST_CASE_REGISTER(InpasCrcTest, test);
	TEST_CASE_REGISTER(InpasCrcTest, testSoh);
	TEST_CASE_REGISTER(InpasCrcTest, testTlv);
	TEST_CASE_REGISTER(InpasCrcTest, testTlvPacketMaker);
	TEST_CASE_REGISTER(InpasCrcTest, testUsbPacketCrc);
//	TEST_CASE_REGISTER(InpasCrcTest, testUnitexQrCodeCrc);
}

bool InpasCrcTest::test() {
	Inpas::Crc crc1;
	crc1.start();
	crc1.add(0x02);
	crc1.add(0x05);
	crc1.add(0x00);
	crc1.add(0x19);
	crc1.add(0x02);
	crc1.add(0x00);
	crc1.add(0x32);
	crc1.add(0x31);
	TEST_NUMBER_EQUAL(0xA5C2, crc1.getCrc());
	TEST_NUMBER_EQUAL(0xA5, crc1.getHighByte());
	TEST_NUMBER_EQUAL(0xC2, crc1.getLowByte());

	Inpas::Crc crc2;
	crc2.start();
	crc2.add(0x02);
	crc2.add(0x15);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x08);
	crc2.add(0x00);
	crc2.add(0x30);
	crc2.add(0x30);
	crc2.add(0x30);
	crc2.add(0x30);
	crc2.add(0x30);
	crc2.add(0x31);
	crc2.add(0x30);
	crc2.add(0x30);
	crc2.add(0x04);
	crc2.add(0x03);
	crc2.add(0x00);
	crc2.add(0x36);
	crc2.add(0x34);
	crc2.add(0x33);
	crc2.add(0x19);
	crc2.add(0x01);
	crc2.add(0x00);
	crc2.add(0x31);
	TEST_NUMBER_EQUAL(0x462E, crc2.getCrc());
	TEST_NUMBER_EQUAL(0x46, crc2.getHighByte());
	TEST_NUMBER_EQUAL(0x2E, crc2.getLowByte());
	return true;
}

bool InpasCrcTest::testSoh() {
	const char hex1[] =
			"01FB0108000101003102010032202A2A"
			"2A2A20383934330D0AD1D0CECA20C4C5"
			"C9D1D2C2C8DF203A2020202020202020"
			"2020202031382F30380D0ACACBC8C5CD"
			"D23A2020202020202020202020202020"
			"2020202020322F544553540D0A414944"
			"20202020202020202020202020202041"
			"303030303030303033313031300D0AD1"
			"D3CCCCC02028525542290D0A20202020"
			"20202020202020202020202020202020"
			"2020202020203130302E30300D0A2020"
			"20202020202020202020CEC4CEC1D0C5"
			"CDCE2020202020202020202020200D0A"
			"CACEC420CED2C2C5D2C0202020202020"
			"20202020202020202020202020203030"
			"0D0ACACEC420C0C2D2CED0C8C7C0D6C8"
			"C83A2020202020202020202037363935"
			"33370D0AB920D1D1DBCBCAC83A202020"
			"20202020202020203835363636363730"
			"333133300D0A200D0A0D0A2D2D2D2D2D"
			"2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D"
			"2D2D2D2D2D2D2D2D2D2D0D0A20202020"
			"202020202020202028CAC0D1D1C8D029"
			"0D0A0D0A3D3D3D3D3D3D3D3D3D3D3D3D"
			"3D3D3D3D3D3D3D3D3D3D3D3D3D3D3D3D"
			"3D3D3D3D0D0A0A0A0A0A0A0A0A0D0A7E"
			"307844415E5E20202020202020202020"
			"20C4C5CCCE20D0C5C6C8CC0A0A0D0A20"
			"2020202020202020202020204D657472"
			"6F204326430D0A202020202020CFD0CE"
			"D6C5D1D1C8CDC3CEC2DBC920D6C5CDD2"
			"D02020202020200D0A202020CCCE";//3FDF
	uint8_t data1[2048];
	uint16_t data1Len = hexToData(hex1, strlen(hex1), data1, sizeof(data1));
	Inpas::Crc crc1;
	crc1.start();
	crc1.add(data1, data1Len);
	TEST_NUMBER_EQUAL(0x3FDF, crc1.getCrc());
	TEST_NUMBER_EQUAL(0x3F, crc1.getHighByte());
	TEST_NUMBER_EQUAL(0xDF, crc1.getLowByte());

	const char hex2[] =
			"01FB0108000101003102010031000500"
			"3130303030040300363433060E003230"
			"3230303531363032323635330A10002A"
			"2A2A2A2A2A2A2A2A2A2A2A383934330B"
			"0400313830380D06003736393533370E"
			"0C003835363636363730333133300F02"
			"003030130800CEC4CEC1D0C5CDCE150E"
			"00323032303035313630323236353317"
			"02002D31190100311A02002D311B0800"
			"34303030303137321C0A003131313131"
			"3131313131270100315ADD0530784446"
			"5E5E2020202020202020202020C4C5CC"
			"CE20D0C5C6C8CC0A0A0D0A2020202020"
			"20202020202020204D6574726F204326"
			"430D0A202020202020CFD0CED6C5D1D1"
			"C8CDC3CEC2DBC920D6C5CDD2D0202020"
			"2020200D0A202020CCCED1CAC2C020D3"
			"CB2ECECAD2DFC1D0DCD1CAC0DF20C42E"
			"37322020200D0A202020202020202020"
			"20D22E203732312D33362D3231202020"
			"202020202020200D0AD7C5CA20CACBC8"
			"C5CDD2C0202020202020202020202020"
			"2020202020303139300D0A2020202020"
			"2020202020CECFCBC0D2C020CFCECAD3"
			"CFCAC80D0A31362E30352E3230202020"
			"2020202020202020202020202030323A"
			"32363A35330D0AD2C5D0CCC8CDC0CB3A"
			"20202020202020202020202020202034"
			"303030303137320D0ACAC0D0D2C02020"
			"20202020202020202020202020564953"
			"4120436C61737369630D0A2020202020"
			"2A2A2A2A202A2A2A2A202A2A2A2A";//6E95
	uint8_t data2[2048];
	uint16_t data2Len = hexToData(hex2, strlen(hex2), data2, sizeof(data2));
	Inpas::Crc crc2;
	crc2.start();
	crc2.add(data2, data2Len);
	TEST_NUMBER_EQUAL(0x6E95, crc2.getCrc());
	TEST_NUMBER_EQUAL(0x6E, crc2.getHighByte());
	TEST_NUMBER_EQUAL(0x95, crc2.getLowByte());
	return true;
}

/*
FIELD(x00;=credit)LEN(x03;x00;)VALUE(x31;x30;x30;=100)
FIELD(x04;=currency)LEN(x03;x00;)VALUE(x36;x34;x33;=643)
FIELD(x06;=host-datetime)LEN(x0E;x00;=14)VALUE(x32;x30;x31;x38;x31;x30; x31;x36; x31;x39; x31;x35; x32;x33;=2018-10-16 19:15:23)
FIELD(x0A;=card-number)LEN(x10;x00;=16)VALUE(x2A;x2A;x2A;x2A; x2A;x2A;x2A;x2A; x2A;x2A;x2A;x2A; x36;x36;x33;x39;=************6639)
FIELD(x0B;=)LEN(x04;x00;=4)VALUE(x31;x39;x30;x33;) WTF?
FIELD(x0D;=auth-code)LEN(x06;x00;=6)VALUE(x32;x34;x37;x36;x32;x39;=247629) WTF?
FIELD(x0E;=ref-number)LEN(x0C;x00;=12)VALUE(x38;x32;x38;x39;x31; x33;x33;x34;x39;x33; x37;x33;=82891333493) WTF?
FIELD(x0F;=host-repsonse-code)LEN(x02;x00;=2)VALUE(x30;x30;=00) WTF?
FIELD(x13;=additional-data)LEN(x08;x00;=8)VALUE(xCE;xC4;xCE;xC1;xD0;xC5;xCD;xCE;=CP1251(ОДОБРЕНО))
FIELD(x15;=terminal-datetime)LEN(x0E;x00;=14)VALUE(x32;x30;x31;x38; x31;x30; x31;x36; x31;x39; x31;x35; x32;x33;=2018-10-16 19:15:23)
FIELD(x17;=transaction-id)LEN(x02;x00;=2)VALUE(x2D;x31;) WTF? ERROR?
FIELD(x19;=operation-code)LEN(x01;x00;=1)VALUE(x31;=payment)
FIELD(x1A;=terminal-transaction-id)LEN(x02;x00;=2)VALUE(x2D;x31;)
FIELD(x1B;=terminal-id)LEN(x08;x00;=8)VALUE(x30;x30;x32;x36;x36;x32;x38;x36;=00266286)
FIELD(x1C;=merchant-id)LEN(x09;x00;=9)VALUE(x31;x31;x31;x31;x31;x31;x31;x31;x31;=111111111)
FIELD(x27;=transaction-result)LEN(x01;x00;=1)VALUE(x31;=1(SUCCESS))
 */
bool InpasCrcTest::testTlv() {
	uint8_t data[] = {
		0x00, 0x03, 0x00, 0x31,  0x30, 0x30, 0x04, 0x03,  0x00, 0x36, 0x34, 0x33,  0x06, 0x0E, 0x00, 0x32,
		0x30, 0x31, 0x38, 0x31,  0x30, 0x31, 0x36, 0x31,  0x39, 0x31, 0x35, 0x32,  0x33, 0x0A, 0x10, 0x00,
		0x2A, 0x2A, 0x2A, 0x2A,  0x2A, 0x2A, 0x2A, 0x2A,  0x2A, 0x2A, 0x2A, 0x2A,  0x36, 0x36, 0x33, 0x39,
		0x0B, 0x04, 0x00, 0x31,  0x39, 0x30, 0x33, 0x0D,  0x06, 0x00, 0x32, 0x34,  0x37, 0x36, 0x32, 0x39,
		0x0E, 0x0C, 0x00, 0x38,  0x32, 0x38, 0x39, 0x31,  0x33, 0x33, 0x34, 0x39,  0x33, 0x37, 0x33, 0x0F,
		0x02, 0x00, 0x30, 0x30,  0x13, 0x08, 0x00, 0xCE,  0xC4, 0xCE, 0xC1, 0xD0,  0xC5, 0xCD, 0xCE, 0x15,
		0x0E, 0x00, 0x32, 0x30,  0x31, 0x38, 0x31, 0x30,  0x31, 0x36, 0x31, 0x39,  0x31, 0x35, 0x32, 0x33,
		0x17, 0x02, 0x00, 0x2D,  0x31, 0x19, 0x01, 0x00,  0x31, 0x1A, 0x02, 0x00,  0x2D, 0x31, 0x1B, 0x08,
		0x00, 0x30, 0x30, 0x32,  0x36, 0x36, 0x32, 0x38,  0x36, 0x1C, 0x09, 0x00,  0x31, 0x31, 0x31, 0x31,
		0x31, 0x31, 0x31, 0x31,  0x31, 0x27, 0x01, 0x00,  0x31
	};
	Inpas::TlvPacket packet(20);
	TEST_NUMBER_EQUAL(true, packet.parse(data, sizeof(data)));

	uint16_t num1;
	TEST_NUMBER_EQUAL(true, packet.getNumber(0x00, &num1));
	TEST_NUMBER_EQUAL(100, num1);
	uint16_t num2;
	TEST_NUMBER_EQUAL(true, packet.getNumber(0x04, &num2));
	TEST_NUMBER_EQUAL(643, num2);
	uint32_t num3;
	TEST_NUMBER_EQUAL(true, packet.getNumber(0x1B, &num3));
	TEST_NUMBER_EQUAL(266286, num3);
	uint32_t num4;
	TEST_NUMBER_EQUAL(true, packet.getNumber(0x27, &num4));
	TEST_NUMBER_EQUAL(1, num4);

	StringBuilder str1;
	TEST_NUMBER_EQUAL(true, packet.getString(0x0A, &str1));
	TEST_STRING_EQUAL("************6639", str1.getString());
	StringBuilder str2;
	TEST_NUMBER_EQUAL(true, packet.getString(0x1B, &str2));
	TEST_STRING_EQUAL("00266286", str2.getString());

	DateTime date1;
	TEST_NUMBER_EQUAL(true, packet.getDateTime(0x06, &date1));
	TEST_NUMBER_EQUAL(18, date1.year);
	TEST_NUMBER_EQUAL(10, date1.month);
	TEST_NUMBER_EQUAL(16, date1.day);
	TEST_NUMBER_EQUAL(19, date1.hour);
	TEST_NUMBER_EQUAL(15, date1.minute);
	TEST_NUMBER_EQUAL(23, date1.second);
	return true;
}

/*
FIELD(x00;=credit)LEN(x08;x00;)VALUE(x30;x30;x30;x30;x30;x31;x30;x30;=00000100)
FIELD(x04;=currency)LEN(x03;x00;)VALUE(x36;x34;x33;=643)
FIELD(x19;=operation_code)LEN(x01;x00;)VALUE(x31;=payment)
 */
bool InpasCrcTest::testTlvPacketMaker() {
	Inpas::TlvPacketMaker maker(256);
	uint32_t num1 = 100;
	TEST_NUMBER_EQUAL(true, maker.addNumber(0x00, 8, num1));
	uint32_t num2 = 643;
	TEST_NUMBER_EQUAL(true, maker.addNumber(0x04, 3, num2));
	uint32_t num3 = 1;
	TEST_NUMBER_EQUAL(true, maker.addNumber(0x19, 1, num3));

	TEST_HEXDATA_EQUAL("000800303030303031303004030036343319010031", maker.getData(), maker.getDataLen());
	return true;
}


static unsigned char PAX_D200_GetXOR(uint8_t *buf, unsigned int len)
{
	unsigned char a;
	unsigned int i;

	for(i=0,a=0;i<len;i++)
	  a^=buf[i];

	return a;
}

bool InpasCrcTest::testUsbPacketCrc() {
	uint8_t data[] = {
		0x06, 0x01, 0xbb, 0x00,
		0x02, 0xb5, 0x00,
		0x00, 0x03, 0x00, 0x31, 0x30, 0x30, 0x04, 0x03, 0x00, 0x36,
		0x34, 0x33, 0x06, 0x0e, 0x00, 0x32, 0x30, 0x31, 0x39, 0x30,
		0x37, 0x30, 0x31, 0x32, 0x31, 0x32, 0x37, 0x30, 0x35, 0x0a,
		0x12, 0x00, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
		0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x31, 0x39, 0x31, 0x30, //050
		0x0b, 0x04, 0x00, 0x32, 0x30, 0x31, 0x31, 0x0d, 0x06, 0x00,
		0x31, 0x36, 0x38, 0x34, 0x31, 0x39, 0x0e, 0x0c, 0x00, 0x30,
		0x30, 0x30, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x32, 0x36,
		0x37, 0x0f, 0x02, 0x00, 0x30, 0x30, 0x13, 0x21, 0x00, 0xce,
		0xcf, 0xc5, 0xd0, 0xc0, 0xd6, 0xc8, 0xdf, 0x20, 0xcf, 0xd0, //100
		0xc5, 0xd0, 0xc2, 0xc0, 0xcd, 0xc0, 0x5e, 0x54, 0x45, 0x52,
		0x4d, 0x49, 0x4e, 0x41, 0x54, 0x45, 0x44, 0x2e, 0x4a, 0x50,
		0x47, 0x7e, 0x15, 0x0e, 0x00, 0x32, 0x30, 0x31, 0x39, 0x30,
		0x37, 0x30, 0x31, 0x32, 0x31, 0x32, 0x37, 0x30, 0x35, 0x17,
		0x02, 0x00, 0x2d, 0x31, 0x19, 0x01, 0x00, 0x31, 0x1a, 0x02, //150
		0x00, 0x2d, 0x31, 0x1b, 0x08, 0x00, 0x30, 0x30, 0x30, 0x36,
		0x30, 0x30, 0x35, 0x36, 0x1c, 0x09, 0x00, 0x31, 0x31, 0x31,
		0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x27, 0x02, 0x00, 0x35,
		0x33, 0x1d, 0x35
	};//0x54

	TEST_NUMBER_EQUAL(0x54, PAX_D200_GetXOR(data, sizeof(data)));
	return true;
}

/*
0003 0001 19-06-05 19:06:05 19:06:05 000066 000201 01 63fe // эспрессо 05.06.2019 21:44
0003 0001 19-06-05 19:06:05 19:06:05 000067 000203 01 65b6 // американо 05.06.2019 21:44
0003 0001 19-06-05 19:06:05 19:06:05 000065 000203 01 e975 // американо 05.06.2019 21:44
0003 0001 19-06-05 19:06:05 19:06:05 000059 000201 01 e3e2 // эспрессо 05.06.2019 21:42
 */
/*
procedure ByteCrc(data:byte; var crc:word);
var i:byte;
begin
 for i:=0 to 7 do
  begin
   if (((crc and $8000) shr 8) xor (data and $80)) <> 0  then
	crc:=(crc shl 1) xor $8005
	else crc:=(crc shl 1);
   data:=data shl 1;
  end;
end;
 */
class UnitexCrc {
public:
	void start() {
		crc = 0;
	}

	void add(uint8_t byte) {
		for(uint8_t i = 0; i < 8; i++) {
			if((((crc & 0x8000) >> 8) ^ (byte & 0x80)) > 0) {
				crc = (crc << 1) ^ 0x8005;
			} else {
				crc = (crc << 1);
			}
			byte = byte << 1;
		}
	}

	void add(uint8_t *data, uint16_t dataLen) {
		for(uint16_t i = 0; i < dataLen; i++) {
			add(data[i]);
		}
	}

	uint16_t getCrc() {
		return crc;
	}

private:
	uint16_t crc;
};

bool InpasCrcTest::testUnitexQrCodeCrc() {
	//0003000119060519060519060500005900020101 e3e2
	uint8_t data1[] = {
		0x00, 0x03, 0x00, 0x01, 0x19, 0x06, 0x05, 0x19,
		0x06, 0x05, 0x19, 0x06, 0x05, 0x00, 0x00, 0x59,
		0x00, 0x02, 0x01, 0x01 }; //e3e2*/
/*	uint8_t data1[] = {
		0x30, 0x30, 0x30, 0x33, 0x30, 0x30, 0x30, 0x31,
		0x31, 0x39, 0x30, 0x36, 0x30, 0x35, 0x31, 0x39,
		0x30, 0x36, 0x30, 0x35, 0x31, 0x39, 0x30, 0x36,
		0x30, 0x35, 0x30, 0x30, 0x30, 0x30, 0x35, 0x39,
		0x30, 0x30, 0x30, 0x32, 0x30, 0x31, 0x30, 0x31 }; //;x65;x33;x65;x32;x0D;x0A;*/
/*	uint8_t data1[] = {
		00, 03, 00, 01, 19, 06, 05, 19,
		06, 05, 19, 06, 05, 00, 00, 59,
		00, 02, 01, 01 }; //e3e2*/
/*	uint8_t data1[] = {
		0, 0, 0, 3, 0, 0, 0, 1, 1, 9, 0, 6, 0, 5, 1, 9,
		0, 6, 0, 5, 1, 9, 0, 6, 0, 5, 0, 0, 0, 0, 5, 9,
		0, 0, 0, 2, 0, 1, 0, 1 }; //e3e2*/
	UnitexCrc crc;
	crc.start();
	crc.add(data1, sizeof(data1));
//	TEST_NUMBER_EQUAL(0xe3e2, crc.getCrc());
	uint16_t crcValue = crc.getCrc();
	TEST_HEXDATA_EQUAL("e3e2", (uint8_t*)&crcValue, 2);
	return true;
}

/*
var i:integer;
	buf,tester,tester2:string;
	k,len,rs:word;
Begin
	inf:='#39#39';
	buf:=[Orders.VisitOtherExtraInfo];
	if length(buf)>20 then buf:=copy(buf,1,20);
	inf:=trim(inf)+trim(AnsiToUtf8(buf));
	inf:=trim(inf)+'#39'1234'#39'+'#39'4321'#39';

	buf:=(FormatDateTime('#39'yy/mm/dd'#39', [Orders.OpenTime]));
	k:=length(buf);
	for i:=1 to k do
		if buf[i]='#39'.'#39' then delete(buf,i,1);
	inf:=trim(inf)+trim(AnsiToUtf8(buf));

	buf:=(FormatDateTime('#39'yy/mm/dd'#39', [Orders.OpenTime]));
	k:=length(buf);
	for i:=1 to k do
		if buf[i]='#39'.'#39' then delete(buf,i,1);
	inf:=trim(inf)+trim(AnsiToUtf8(buf));

	buf:=(FormatDateTime('#39'yy/mm/dd'#39', [Orders.OpenTime]+30));
	k:=length(buf);
	for i:=1 to k do
		if buf[i]='#39'.'#39' then delete(buf,i,1);
	inf:=trim(inf)+trim(AnsiToUtf8(buf));


	buf:=inttoStr([DishUNI]);
	if length(buf)>6 then buf:=copy(buf,length(buf)-6,6) else
	if (length(buf)<6) then
	begin   '
		for i:=length(buf)+1 to 6 do buf:='#39'0'#39'+buf;
	end;
	inf:=trim(inf)+trim(buf);

	buf:=[Dish.CLASSIFICATORGROUPS^2560.Code];
	if length(buf)>2 then buf:=copy(buf,length(buf)-2,2) else

	if (length(buf)<1) then for i:=length(buf)+1 to 2 do buf:='#39'0'#39'+b' + uf;
	inf:=trim(inf)+trim(AnsiToUtf8(buf));

	buf:=intToStr([DishCode]);
	if length(buf)>4 then buf:=copy(buf,length(buf)-4,4) else

	if (length(buf)<4) then for i:=length(buf)+1 to 4 do buf:='#39'0'#39'+b' + uf;
	inf:=trim(inf)+trim(AnsiToUtf8(buf));

	buf:=intToStr(round([Quantity]));
	if length(buf)>99 then buf:='#39'00'#39' else
	if length(buf)<10 then buf:='#39'0'#39'+buf;
	inf:=trim(inf)+trim(AnsiToUtf8(buf));
//memo1.text:=Utf8ToAnsi(inf);
// inf:='#39'1234'#39';
	rs:=$FFFF;
	len:=length(inf);
	for i:=1 to len do
	begin
		bytecrc(ord(inf[i]),rs);
	end;
	inf:=inf+intToHex(rs,4);
// memo1.text:=Utf8ToAnsi(inf);
//inf:=trim(nf)+intToHex(rs,2);
end
 */
