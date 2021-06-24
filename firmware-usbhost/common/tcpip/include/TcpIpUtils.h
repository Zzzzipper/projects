#ifndef LIB_TCPIP_UTILS_H
#define LIB_TCPIP_UTILS_H

#include "utils/include/Event.h"
#include "utils/include/StringBuilder.h"

#define IPADDR_STRING_SIZE 15
#define IPADDR4TO1(_a, _b, _c, _d) (_a & 0xFF) | ((_b & 0xFF) << 8) | ((_c & 0xFF) << 16) | ((_d & 0xFF) << 24)
#define LOG_IPADDR(_addr) (_addr & 0xFF) << "." << ((_addr >> 8) & 0xFF) << "." << ((_addr >> 16) & 0xFF) << "." << ((_addr >> 24) & 0xFF)
#define IPADDR_FORMAT(_addr) (_addr & 0xFF),((_addr >> 8) & 0xFF),((_addr >> 16) & 0xFF),((_addr >> 24) & 0xFF)
#define IPADDR_QARG(_addr) arg(_addr & 0xFF).arg((_addr >> 8) & 0xFF).arg((_addr >> 16) & 0xFF).arg((_addr >> 24) & 0xFF)

extern bool stringToIpAddr(const char *string, uint32_t *ipaddr);

#endif
