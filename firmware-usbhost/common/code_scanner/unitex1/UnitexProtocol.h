#ifndef LIB_SALEMANAGER_CCI_UNITEXPROTOCOL_H_
#define LIB_SALEMANAGER_CCI_UNITEXPROTOCOL_H_

#include "utils/include/NetworkProtocol.h"

namespace Unitex {

#pragma pack(push,1)
////0003 0001 19 06 05 19 06 05 19 06 05 000066 000201 01 63fe
struct CodeRequest {
	LEUnum4 param1;
	LEUnum4 param2;
	LEUnum2 year1;
	LEUnum2 month1;
	LEUnum2 day1;
	LEUnum2 year2;
	LEUnum2 month2;
	LEUnum2 day2;
	LEUnum2 year3;
	LEUnum2 month3;
	LEUnum2 day3;
	LEUnum6 checkNum;
	LEUnum6 cashlessId;
	LEUnum2 param3;
	uint8_t crc[4];
};
#pragma pack(pop)

}

#endif
