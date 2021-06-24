#include "utils/include/CodePage.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class CodePageTest : public TestSet {
public:
	CodePageTest();
	bool testConvertWin1251ToUtf8();
	bool testConvertUtf8ToWin1251();
	bool testConvertWin1251ToJsonUnicode();
	bool testConvertJsonUnicodeToWin1251();
	bool testConvertBinToBase64();
	bool testConvertBase64ToBin();
};

TEST_SET_REGISTER(CodePageTest);

CodePageTest::CodePageTest() {
	TEST_CASE_REGISTER(CodePageTest, testConvertWin1251ToUtf8);
	TEST_CASE_REGISTER(CodePageTest, testConvertUtf8ToWin1251);
	TEST_CASE_REGISTER(CodePageTest, testConvertWin1251ToJsonUnicode);
	TEST_CASE_REGISTER(CodePageTest, testConvertJsonUnicodeToWin1251);
	TEST_CASE_REGISTER(CodePageTest, testConvertBinToBase64);
	TEST_CASE_REGISTER(CodePageTest, testConvertBase64ToBin);
}

bool CodePageTest::testConvertWin1251ToUtf8() {
	const char src[] = "ABCabc¿®‘‡∏Ù";
	uint8_t dst[256];
	uint16_t dstLen = convertWin1251ToUtf8((const uint8_t*)src, strlen(src), dst, sizeof(dst));
	TEST_HEXDATA_EQUAL("414243616263D090D081D0A4D0B0D191D184", dst, dstLen);
	return true;
}

bool CodePageTest::testConvertUtf8ToWin1251() {
	const uint8_t src[] = { 0x41, 0x42, 0x43, 0x61, 0x62, 0x63, 0xD0, 0x90, 0xD0, 0x81, 0xD0, 0xA4, 0xD0, 0xB0, 0xD1, 0x91, 0xD1, 0x84 };
	uint8_t dst[256];
	uint16_t dstLen = convertUtf8ToWin1251(src, sizeof(src), dst, sizeof(dst));
	TEST_SUBSTR_EQUAL("ABCabc¿®‘‡∏Ù", (const char*)dst, dstLen);
	return true;
}

bool CodePageTest::testConvertWin1251ToJsonUnicode() {
	// spec symbols
	const char src1[] = "ABCabc¿¡¬‡·‚\" \\ / \b \f \n \r \t´ª";
	StringBuilder dst1(1024);
	convertWin1251ToJsonUnicode(src1, &dst1);
	TEST_STRING_EQUAL("ABCabc\\u0410\\u0411\\u0412\\u0430\\u0431\\u0432\\\" \\\\ \\/ \\b \\f \\n \\r \\t\\u00AB\\u00BB", dst1.getString());

	// undefined symbol 0x98
	const uint8_t src2[] = { 0x30, 0x98, 0x31, 0x00 };
	StringBuilder dst2(1024);
	convertWin1251ToJsonUnicode((char*)src2, &dst2);
	TEST_STRING_EQUAL("01", dst2.getString());

	// control symbols
	const uint8_t src3[] = { 0x31, 0x01, 0x32, 0x02, 0x33, 0x03, 0x34, 0x0A, 0x35, 0x7F, 0x36, 0x00 };
	StringBuilder dst3(1024);
	convertWin1251ToJsonUnicode((char*)src3, &dst3);
	TEST_STRING_EQUAL("1234\\n56", dst3.getString());
	return true;
}

bool CodePageTest::testConvertJsonUnicodeToWin1251() {
	const char src1[] = "ABCabc\\u0410\\u0411\\u0412\\u0430\\u0431\\u0432\\u9999\\r\\n";
	StringBuilder dst1(1024);
	convertJsonUnicodeToWin1251(src1, strlen(src1), &dst1);
	TEST_STRING_EQUAL("ABCabc¿¡¬‡·‚?\r\n", dst1.getString());
	return true;
}

bool CodePageTest::testConvertBinToBase64() {
	uint8_t src1[] = {
		0x00, 0x13, 0xc0, 0x0c, 0x00, 0x1d, 0x44, 0x0a, 0x00, 0xd3,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0xdb, 0xd5};
	Buffer dst1(1024);
	uint16_t dst1Len = convertBinToBase64(src1, sizeof(src1), dst1.getData(), dst1.getSize());
	TEST_HEXDATA_EQUAL("414250414441416452416F41307741414141414141414142414141413239553D", dst1.getData(), dst1Len);
	TEST_SUBSTR_EQUAL("ABPADAAdRAoA0wAAAAAAAAABAAAA29U=", (char*)dst1.getData(), dst1Len);

	uint8_t src2[] = {
		0x00, 0x07, 0x50, 0x00, 0x00, 0xe4, 0xdf, 0x03, 0x00, 0x10, 0x67};
	Buffer dst2(1024);
	uint16_t dst2Len = convertBinToBase64(src2, sizeof(src2), dst2.getData(), dst2.getSize());
	TEST_HEXDATA_EQUAL("414164514141446B33774D414547633D", dst2.getData(), dst2Len);
	TEST_SUBSTR_EQUAL("AAdQAADk3wMAEGc=", (char*)dst2.getData(), dst2Len);
	return true;
}

bool CodePageTest::testConvertBase64ToBin() {
	Buffer src1(256);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("414164514141446B33774D414547633D", &src1));
	Buffer dst1(256);
	uint16_t dst1Len = convertBase64ToBin(src1.getData(), src1.getLen(), dst1.getData(), dst1.getLen());
	TEST_HEXDATA_EQUAL("0007500000E4DF03001067", dst1.getData(), dst1Len);
	return true;
}
