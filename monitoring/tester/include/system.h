#pragma once

#include <cstdint>

namespace tester {
class System {
public:
    explicit System();

    /// Get total virtual memory
    double totalVirtualMemory();

    /// Total Physical Memory (RAM)
    double totalPhisicalMemory();

    /// Current virtual memory size
    double currentlyUsedVirtualMemory();

    /// Physical Memory currently used
    double currentlyUsedPhisicalMemory();

    /// Virtual Memory currently used by current process
    /// Note: this value is in KB!
    double currentProcessVirtualMemoryUsed();

    /// Physical Memory currently used by current process
    double currentProcessPhisicalMemoryUsed();

    /// Memory data currently used by current process
    double currentProcessMemoryDataUsed();

    /// CPU currently used
    double cpuCurrentlyUsed();

    /// CPU currently used by current process
    double cpuCurrentlyUsedByCurrentProcess();

private:
    /// This assumes that a digit will be
    /// found and the line ends in " Kb".
    double _parseLine(char* line);

    /// Read cpu stat
    void _initCpuStat();

    /// Read cpu stat on creating object
    void _initCpuInfo();

};
}
