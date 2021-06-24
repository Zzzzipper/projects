#include "include/TcpIpUtils.h"

#include "common/utils/include/StringParser.h"

bool stringToIpAddr(const char *string, uint32_t *ipaddr) {
	StringParser parser(string);
	uint8_t num1 = 0;
	if(parser.getNumber(&num1) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint8_t num2 = 0;
	if(parser.getNumber(&num2) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint8_t num3 = 0;
	if(parser.getNumber(&num3) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint8_t num4 = 0;
	if(parser.getNumber(&num4) == false) {
		return false;
	}
	*ipaddr = num1 | (num2 << 8) | (num3 << 16) | (num4 << 24);
//	IP4_ADDR(ipaddr, num1, num2, num3, num4);
	return true;
}
