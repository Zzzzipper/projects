#ifndef COMMON_UTILS_VERSION_H
#define COMMON_UTILS_VERSION_H

#define VERSION_MAJOR(version) ((version >> 24) & 0xFF)
#define VERSION_MINOR(version) ((version >> 16) & 0xFF)
#define VERSION_BUILD(version) (version & 0xFFFF)
#define LOG_VERSION(version) VERSION_MAJOR(version) << "." << VERSION_MINOR(version) << "." << VERSION_BUILD(version)

// Версии железа и прошивки. Прописаны в arm-gcc-link.ld
extern unsigned long HardwareVersion;
extern unsigned long SoftwareVersion;

extern bool stringToVersion(const char *data, uint16_t dataLen, uint32_t *result);

#endif
