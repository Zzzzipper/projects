#include "LogTargetFile.h"

LogTargetFile::LogTargetFile() {
}

LogTargetFile::~LogTargetFile() {

}

bool LogTargetFile::open(const char *filename) {
	close();
	file.setFileName(filename);
	return file.open(QIODevice::ReadWrite);
}

void LogTargetFile::send(const uint8_t *data, const uint16_t len) {
	file.write((const char*)data, len);
	file.flush();
}

void LogTargetFile::close() {
	file.close();
}
