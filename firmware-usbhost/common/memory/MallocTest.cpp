#include "include/MallocTest.h"
#ifdef DEBUG_MEMORY

#include "platform/include/platform.h"
#include "logger/include/Logger.h"

#include <malloc.h>

#ifndef HEAP_TEST_LIST_SIZE
#define HEAP_TEST_LIST_SIZE 0
#endif

static int memOffset = 0x20000000;
static int memSize = HEAP_TEST_LIST_SIZE;
static int heapHead = 0;
static int heapPeak = 0;
static int memUsed = 0;
static int memPeak = 0;
static int mallocNum = 0;
static int reallocNum = 0;
static int freeNum = 0;
static int errorMallocSize = 0;
static int errorMallocHeapData = 0;
static int errorMallocHeapFrag = 0;
static int errorListFull = 0;
static int errorNotFound = 0;
struct MemoryData {
	unsigned int ptr;
	unsigned int size;
} mem[HEAP_TEST_LIST_SIZE];


/*
void MallocInit() {
	memSize = mallocSize;
	mem = (MemoryData*)malloc(sizeof(MemoryData) * memSize);
	MallocReset();
}
*/

void MallocReset() {
	LOG("#### MEMORY RESET");
	memUsed = 0;
	memPeak = 0;
	mallocNum = 0;
	reallocNum = 0;
	freeNum = 0;
	for(int i = 0; i < memSize; i++) {
		mem[i].ptr = 0;
		mem[i].size = 0;
	}
}

int MallocFind(void *ptr) {
	for(int i = 0; i < memSize; i++) {
		if(mem[i].ptr == (unsigned int)ptr) {
			return i;
		}
	}
	return memSize;
}

void MallocAdd(void *ptr, unsigned int size) {
	if(memUsed <= 0) {
		for(int i = 0; i < memSize; i++) {
			mem[i].ptr = 0;
			mem[i].size = 0;
		}
	}

	if(ptr == NULL) {
		errorMallocSize = size;
#ifdef ARM
		struct mallinfo info = mallinfo();
		errorMallocHeapData = (unsigned int)info.uordblks;
		errorMallocHeapFrag = (unsigned int)info.fordblks;
#endif
		return;
	}

	heapHead = (unsigned int)ptr + size - memOffset;
	if(heapHead > heapPeak) {
		heapPeak = heapHead;
	}

	int i = MallocFind(0);
	if(i == memSize) {
		errorListFull++;
		return;
	}
	mem[i].ptr = (unsigned int)ptr;
	mem[i].size = size;
	memUsed += size;
	if(memUsed > memPeak) {
		memPeak = memUsed;
	}
}

void MallocRemove(void *ptr) {
	int i = MallocFind(ptr);
	if(i == memSize) {
		errorNotFound++;
		return;
	}
	memUsed -= mem[i].size;
	mem[i].ptr = 0;
	mem[i].size = 0;
}

void MallocPrintInfo() {
#ifdef ARM
	struct mallinfo info = mallinfo();
	LOG(">>>> arena =" << (uint32_t)info.arena);
	LOG(">>>> uordblks =" << (uint32_t)info.uordblks);
	LOG(">>>> fordblks =" << (uint32_t)info.fordblks);
#endif
	LOG("#### heapHead=" << heapHead);
	LOG("#### heapPeak=" << heapPeak);
	LOG("#### memUsed=" << memUsed);
	LOG("#### memPeak=" << memPeak);
	LOG("#### mallocCnt=" << mallocNum);
	LOG("#### reallocCnt=" << reallocNum);
	LOG("#### freeCnt=" << freeNum);
	if(errorListFull > 0) { LOG("#### errorListFull=" << errorListFull); }
	if(errorNotFound > 0) { LOG("#### errorNotFound=" << errorNotFound); }
	if(errorMallocSize > 0) { LOG("#### errorMallocSize=" << errorMallocSize); }
	if(errorMallocHeapData > 0) { LOG("#### errorMallocHeapData=" << errorMallocHeapData); }
	if(errorMallocHeapFrag > 0) { LOG("#### errorMallocHeapFrag=" << errorMallocHeapFrag); }
}

void MallocPrintHeap() {
	LOG("#### heap start");
	for(int i = 0; i < memSize; i++) {
		if(mem[i].ptr > 0) {
			*(Logger::get()) << (uint32_t)(mem[i].ptr - memOffset) << ":" << (uint32_t)mem[i].size << Logger::endl;
		}
	}
	LOG("#### heap end");
}

void* MallocMalloc(size_t size) {
	void *ptr = 0;
	ATOMIC {
		ptr = malloc(size);
		MallocAdd(ptr, size);
		mallocNum++;
	}
	return ptr;
}

void* MallocMallocEx(size_t size, const char* func, unsigned int line) {
//	LOG("MALLOC " << (unsigned int)size << "," << func << ":" << (unsigned int)line);
	return MallocMalloc(size);
}

void* MallocRealloc(void* ptr, size_t size) {
    void *ptr2 = 0;
	ATOMIC {
    	ptr2 = realloc(ptr, size);
    	MallocRemove(ptr);
    	MallocAdd(ptr2, size);
    	reallocNum++;
	}
    return ptr2;
}

void* MallocReallocEx(void* ptr, size_t size, const char* func, unsigned int line) {
//	LOG("REALLOC " << (unsigned int)size << "," << func << ":" << (unsigned int)line);
	return MallocRealloc(ptr, size);
}

void MallocFree(void* ptr) {
	ATOMIC {
		free(ptr);
		MallocRemove(ptr);
		freeNum++;
	}
}

void MallocFreeEx(void* ptr, const char* func, unsigned int line) {
//	LOG("FREE " << func << ":" << (unsigned int)line);
	MallocFree(ptr);
}
#endif
