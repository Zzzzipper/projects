#include "system.h"

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <iostream>
#include <sstream>
#include <sys/times.h>
#include <sys/vtimes.h>
#include <unistd.h>

namespace tester {

/**
 * @brief System::System
 */
System::System()
{
    _initCpuStat();
    _initCpuInfo();
}

/**
 * @brief System::totalVirtualMemory - Get total virtual memory
 * @return
 */
double System::totalVirtualMemory() {
    struct sysinfo memInfo;

    sysinfo (&memInfo);
    auto totalVirtualMem = memInfo.totalram;

    // Add other values in next statement to
    // avoid int overflow on right hand side...
    totalVirtualMem += memInfo.totalswap;
    totalVirtualMem *= memInfo.mem_unit;

    return totalVirtualMem/1024/1024.0;
}

/**
 * @brief System::totalPhisicalMemory - Total Physical Memory (RAM)
 * @return
 */
double System::totalPhisicalMemory() {
    struct sysinfo memInfo;

    sysinfo (&memInfo);
    long long totalPhysMem = memInfo.totalram;

    // Multiply in next statement to avoid
    // int overflow on right hand side...
    totalPhysMem *= memInfo.mem_unit;

    return totalPhysMem/1024/1024.0;
}

/**
 * @brief System::currentlyUsedVirtualMemory - Current virtual memory size
 * @return
 */
double System::currentlyUsedVirtualMemory() {
    struct sysinfo memInfo;

    sysinfo (&memInfo);

    auto virtualMemUsed = memInfo.totalram - memInfo.freeram;

    // Add other values in next statement to
    // avoid int overflow on right hand side...
    virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
    virtualMemUsed *= memInfo.mem_unit;

    return virtualMemUsed/1024/1024.0;

}

/**
 * @brief System::currentlyUsedPhisicalMemory - Physical Memory currently used
 * @return
 */
double System::currentlyUsedPhisicalMemory() {
    struct sysinfo memInfo;

    sysinfo (&memInfo);

    auto physMemUsed = memInfo.totalram - memInfo.freeram;

    // Multiply in next statement to avoid
    // int overflow on right hand side...
    physMemUsed *= memInfo.mem_unit;

    return physMemUsed/1024/1024.0;

}

/**
 * @brief System::_parseLine - This assumes that a digit will
 *                             be found and the line ends in " Kb".
 * @param line
 * @return
 */
double System::_parseLine(char* line) {
    uint64_t i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atof(p);
    return i;
}

/**
 * @brief Virtual Memory currently used by current process
 *        Note: this value is in KB!
 * @return
 */
double System::currentProcessVirtualMemoryUsed(){
    std::stringstream s;
    s << "/proc/" << getpid() << "/status";

    FILE* file = fopen(s.str().c_str(), "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmSize:", 7) == 0){
            result = _parseLine(line);
            break;
        }
    }
    fclose(file);
    return result/1024.0;
}

/**
 * @brief - Physical Memory currently used by current process
 * @return
 */
double System::currentProcessPhisicalMemoryUsed(){
    std::stringstream s;
    s << "/proc/" << getpid() << "/status";

    FILE* file = fopen(s.str().c_str(), "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = _parseLine(line);
            break;
        }
    }
    fclose(file);
    return result/1024.0;
}

/**
 * @brief Memory data currently used by current process
 * @return
 */
double System::currentProcessMemoryDataUsed() {
    std::stringstream s;
    s << "/proc/" << getpid() << "/status";

    FILE* file = fopen(s.str().c_str(), "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmData:", 7) == 0){
            result = _parseLine(line);
            break;
        }
    }
    fclose(file);
    return result/1024.0;
}

static uint64_t lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

/**
 * @brief System::_initCpuStat - Read cpu stat
 */
void System::_initCpuStat() {
    FILE* file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
           &lastTotalUser,
           &lastTotalUserLow,
           &lastTotalSys,
           &lastTotalIdle);
    fclose(file);
}


/**
 * @brief CPU currently used
 * @return
 */
double System::cpuCurrentlyUsed() {
    double percent;
    FILE* file;
    uint64_t totalUser, totalUserLow, totalSys, totalIdle, total;

    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
           &totalUser,
           &totalUserLow,
           &totalSys,
           &totalIdle);

    fclose(file);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
            totalSys < lastTotalSys || totalIdle < lastTotalIdle){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
                (totalSys - lastTotalSys);
        percent = total;
        total += (totalIdle - lastTotalIdle);
        percent /= total;
        percent *= 100;
    }

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return percent;
}

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

/**
 * @brief Read cpu stat on creating object
 */
void System::_initCpuInfo() {
    FILE* file;
    struct tms timeSample;
    char line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    file = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while(fgets(line, 128, file) != NULL){
        if (strncmp(line, "processor", 9) == 0) numProcessors++;
    }
    fclose(file);
}

/**
 * @brief CPU currently used by current process
 * @return
 */
double System::cpuCurrentlyUsedByCurrentProcess() {
    struct tms timeSample;
    clock_t now;
    double percent;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
            timeSample.tms_utime < lastUserCPU){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        percent = (timeSample.tms_stime - lastSysCPU) +
                (timeSample.tms_utime - lastUserCPU);
        percent /= (now - lastCPU);
        percent /= numProcessors;
        percent *= 100;
    }

    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return percent;
}



}
