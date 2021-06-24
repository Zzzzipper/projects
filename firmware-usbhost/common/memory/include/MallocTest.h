#ifndef COMMON_MEMORY_MALLOCTEST_H
#define COMMON_MEMORY_MALLOCTEST_H

#include "config.h"

#include <stddef.h>
#include <stdint.h>

#ifdef DEBUG_MEMORY
void MallocInit(int mallocSize);
void MallocReset();
int  MallocFind(void *ptr);
void MallocAdd(void *ptr, uint32_t size);
void MallocRemove(void *ptr);
void MallocPrintInfo();
void MallocPrintHeap();

void* MallocMalloc(size_t size);
void* MallocMallocEx(size_t size, const char* func, unsigned int line);

void* MallocRealloc(void* ptr, size_t size);
void* MallocReallocEx(void* ptr, size_t size, const char* func, unsigned int line);

void MallocFree(void* ptr);
void MallocFreeEx(void* ptr, const char* func, unsigned int line);
#endif
#endif
