#include "FileMemory.h"

#include "logger/include/Logger.h"

FileMemory::FileMemory() {
}

FileMemory::~FileMemory() {
	file.close();
}

bool FileMemory::open(const char *filename) {
	file.setFileName(filename);
	if(file.open(QIODevice::ReadWrite) == false) {
		return false;
	}
	return true;
}

void FileMemory::remove() {
	file.remove();
}

void FileMemory::close() {
	file.close();
}

void FileMemory::setAddress(uint32_t address) {
	file.seek(address);
}

uint32_t FileMemory::getAddress() {
	return file.pos();
}

void FileMemory::skip(uint32_t len) {
	file.seek(file.pos() + len);
}

MemoryResult FileMemory::write(const void *pData, uint32_t len) {
	qint64 writeLen = file.write((char*)pData, len);
	if(writeLen != len) {
		LOG("write error " << writeLen << "/" << len);
		return MemoryResult_WriteError;
	}
	file.flush();
	return MemoryResult_Ok;
}

MemoryResult FileMemory::read(void *pData, uint32_t size) {
	qint64 readLen = file.read((char*)pData, size);
	if(readLen != size) {
		LOG("read error " << readLen << "/" << size);
		return MemoryResult_ReadError;
	}
	return MemoryResult_Ok;
}

uint32_t FileMemory::getMaxSize() const {
	return 1024*1024;
}

uint32_t FileMemory::getPageSize() const {
	return 1;
}
