#pragma once

#include "dblink.h"

namespace receiver {


class DataFlow {
public:
    DataFlow() = default;

    /// Perform pack configs for tester and if has error then return
    /// error code
    uint32_t packConfigForTester(uint32_t, uint32_t, std::vector<uint8_t>&);


    /// Perform upload reports from tester to database
    uint32_t uploadReportsFromTester(uint32_t, std::vector<uint8_t>&);

private:
    /// Perform string array from list conjuncted string array
    std::vector<std::string> _makeResult(std::string& result, char delimiter);

};

} // namespace receiver
