#include "dataflow.h"
#include "log.h"
#include "protocol.h"
#include "config.h"
#include "scope.h"
#include "utils.h"

using namespace receiver;

static const char* _tag = "[DATAFLOW] ";

/**
 * @brief _makeResult - Perform string array from list conjuncted string array
 * @param result
 * @return
 */
std::vector<std::string> DataFlow::_makeResult(std::string& result, char delimiter) {
    result.erase(0,1);
    result.erase(result.length() - 1);
    std::vector<std::string> strings;
    std::istringstream fstream(result);
    std::string string;
    LOG_TRACE << _tag << "---------------------- _makeResult: " << string.length();
    auto counter = 0;
    while (getline(fstream, string, delimiter)) {
        strings.push_back(string);
        LOG_TRACE << _tag << counter << " data: " << string << ", length:" << string.length();
        counter++;
    }
    return strings;
}


/**
 * @brief DataFlow::packConfigForTester - Perform pack configs for
 *        tester and if has error then return error code
 * @param buffer - output buffer
 * @return error code
 */

auto stoll = [](const std::string& string) {
    return (string.empty())? 0: std::stoll(string, nullptr, 10);
};

auto stoi = [](const std::string& string) {
    return (string.empty())? 0: std::stoi(string);
};

uint32_t DataFlow::packConfigForTester(uint32_t testerId, uint32_t ipAddress, std::vector<uint8_t> &buffer) {

    auto dbLinkContainer = receiver::DbLinkContainer::instance().container();

    if(dbLinkContainer.size() > 0) {

        auto recordCount  = 0;
        auto offset = 0;

        auto bufferSize = sizeof(ST_SIZES) + sizeof(ST_T_HDR) + sizeof(TESTER_CFG_ADDDATA);
        buffer.resize(bufferSize, '\0');

        auto iit = dbLinkContainer.begin();
        for(; iit != dbLinkContainer.end(); ++iit) {
            if(iit->second.connect()) {

                LOG_INFO << _tag << "Start pack tasks from " << iit->second.dbAlias() << "(base: " << iit->first << ")" ;

                std::vector<std::string> result;
                try {
                    result = iit->second.execFunc(std::string("select getcfg($1::integer, $2::bigint, $3::text);"),
                                           testerId, ipAddress, "");
                }
                catch(std::exception const &e) {
                    LOG_ERROR << _tag << e.what();
                    continue;
                }

                LOG_DEBUG << _tag << "Tasks row count " << result.size();
                for(auto row: result) {
                    sTesterConfigRecord record;
                    memset(&record, 0, sizeof(sTesterConfigRecord));

                    auto fields = _makeResult(row, ',');

                    offset = bufferSize;

                    record.LObjId        = stoi(fields[0]);
                    record.ModType       = stoi(fields[1]);
                    record.Port          = stoi(fields[2]);
                    record.CheckPeriod   = stoi(fields[3]);
                    record.ResolvePeriod = stoi(fields[4]);
                    record.IP            = stoll(fields[5]);
                    record.NextCheckDt   = stoll(fields[6]);
                    record.PObjId        = stoll(fields[16]);
                    record.ClientId      = stoll(fields[17]);
                    record.TimeOut       = stoll(fields[10]);
                    record.LimWarn       = stoll(fields[12]);
                    record.LimAlrt       = stoll(fields[13]);
                    record.LimDir        = stoll(fields[15]);
                    record.CFGVer        = stoi(fields[11]);
                    record.Flags         = stoi(fields[15]);
                    record.HostNameLen   = fields[7].length();
                    record.baseIndex     = iit->first;
                    record.ConfigLen     = (fields.size() == 19)? fields[18].length(): 0;

                    bufferSize += sizeof(sTesterConfigRecord) + record.HostNameLen + record.ConfigLen;
                    buffer.resize(bufferSize);

                    std::memcpy(buffer.data() + offset, &record, sizeof(sTesterConfigRecord));
                    if(record.HostNameLen) {
                        std::memcpy(buffer.data() + offset + sizeof(sTesterConfigRecord),
                                    fields[7].c_str(), fields[7].length());
                    }
                    if(record.ConfigLen) {
                        std::memcpy(buffer.data() + offset + sizeof(sTesterConfigRecord) + fields[7].length(),
                                fields[18].c_str(), fields[18].length());
                    }

                    TesterConfigRecord classRecord((sTesterConfigRecord*)(buffer.data() + offset));
                    classRecord.print(LOG_TRACE << _tag);

                    LOG_INFO << _tag
                             << "base: " << iit->first
                             << ", type: "<< taskNameById(record.ModType)
                             << ", ip: "  << ipNumericToString(record.IP)
                             << ", obj: " << record.LObjId
                             << ", config: " << ((record.ConfigLen)? fields[18]: "nothing");

                    ++recordCount;
                }
            }
        }

        auto size = buffer.size() - sizeof(ST_SIZES);
        std::memcpy(buffer.data() + sizeof(uint32_t), &size, sizeof(uint32_t));
        std::memcpy(buffer.data() + sizeof(ST_SIZES), &recordCount, sizeof(uint32_t));

        TESTER_CFG_ADDDATA configAdd;
        memset(&configAdd, 0, sizeof(TESTER_CFG_ADDDATA));

        configAdd.CfgRereadPeriod = Config::instance()["reReadPeriod"].ToInt();

        const auto p1 = std::chrono::system_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::seconds>(
                    p1.time_since_epoch()).count();

        configAdd.ServerTime = dt;
        configAdd.ProtocolVersion = 101;
        std::memcpy(buffer.data() + sizeof(ST_SIZES) + sizeof(ST_T_HDR), &configAdd, sizeof(TESTER_CFG_ADDDATA));

        LOG_DEBUG << _tag << "Send " << buffer.size() << " bytes";

        return 0;

    } else {
        return SOCK_RC_SQL_ERROR;
    }

    return -1;
}

/**
 * @brief DataFlow::uploadReportsFromTester - Perform upload reports from tester to database
 * @return
 */
uint32_t DataFlow::uploadReportsFromTester(uint32_t size, std::vector<uint8_t> &packet) {

    auto dbLinkContainer = receiver::DbLinkContainer::instance().container();
    std::set<uint16_t> cleared;

    for(uint32_t i = 0; i < size; ++i) {

        sChgRecord record;
        std::memset(&record, 0, sizeof(sChgRecord));
        std::memcpy(&record, packet.data() + sizeof(ST_SIZES) + sizeof(ST_T_HDR) +
                    i * sizeof(sChgRecord), sizeof(sChgRecord));

        auto link = dbLinkContainer.find(record.baseIndex);
        if(link != dbLinkContainer.end()) {
            if(link->second.connect()) {
                auto nowCleared = cleared.find(record.baseIndex);
                if(nowCleared == cleared.end()) {
                    try {
                        link->second.execFunc(std::string("delete from reports;"));
                        cleared.insert(record.baseIndex);
                        LOG_TRACE << _tag << record.baseIndex << " source table cleaned";
                    }
                    catch (std::exception const &e) {
                        LOG_TRACE << _tag << e.what();
                    }
                }

                LOG_TRACE << _tag << "Received reports to base: " << record.baseIndex;

                try {
                    std::vector<uint8_t> data = {1, 2, 3, 4};
#ifdef BLOB_PQ
                    pqxx::binarystring blob(data.data(), data.size());
#else
                    std::basic_string<std::byte> blob(
                                static_cast<const std::byte *>(static_cast<const void *>(data.data())),
                                data.size()
                                );

#endif
                    LOG_INFO << _tag
                             << "base: " << link->first
                             << ", type: "<< taskNameById(record.ModType)
                             << ", ip: "  << ipNumericToString(record.IP)
                             << ", obj: " << record.LObjId
                             << ", res: " << ((record.CheckRes == 0)? "Success": "Failed");

                    auto result = link->second.execFunc(std::string("call receiver_insert_report($1::bigint, $2::integer,"
                                                                    "$3::bigint, $4::integer, $5::smallint, $6::smallint,"
                                                                    "$7::bigint, $8::bigint, $9::integer, $10::smallint,"
                                                                    "$11::smallint, $12::smallint, $13::integer, $14::smallint,"
                                                                    "$15::smallint, $16::bytea);"),
                                                        static_cast<unsigned int>(record.LObjId),
                                                        static_cast<unsigned int>(record.CheckDt),
                                                        static_cast<unsigned int>(record.IP),
                                                        static_cast<unsigned int>(record.CheckRes),
                                                        static_cast<short unsigned int>(record.DelayMS),
                                                        static_cast<short unsigned int>(record.ModType),
                                                        static_cast<unsigned int>(record.PObjId),
                                                        static_cast<unsigned int>(record.ClientId),
                                                        static_cast<unsigned int>(record.ProblemIP),
                                                        static_cast<short unsigned int>(record.ProblemICMP_Type),
                                                        static_cast<short unsigned int>(record.ProblemICMP_Code),
                                                        static_cast<short unsigned int>(record.CheckStatus),
                                                        static_cast<short unsigned int>(record.Flags),
                                                        static_cast<short unsigned int>(record.CFGVer),
                                                        static_cast<short unsigned int>(record.Port),
                                                        blob);
                    LOG_TRACE << _tag << record.baseIndex << " source table inserted";
                }
                catch(std::exception const &e) {
                    LOG_ERROR << _tag << e.what();
                }
            }
        }
    }

    for(auto jtt = dbLinkContainer.begin(); jtt != dbLinkContainer.end(); ++jtt) {
        if(jtt->second.connect()) {
            try {
                auto result = jtt->second.execFunc(std::string("call update_problem_cart_receiver();"));
            }
            catch(std::exception const &e) {
                LOG_ERROR << _tag << e.what();
            }

            try {
                auto result = jtt->second.execFunc(std::string("call create_problem_cart_receiver();"));
            }
            catch(std::exception const &e) {
                LOG_ERROR << _tag << e.what();
            }
        }
    }


    return 0;
}
