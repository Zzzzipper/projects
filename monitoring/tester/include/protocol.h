#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <array>
#include <iomanip>

#include <openssl/aes.h>
#include <openssl/md5.h>

#include "log.h"

#define DH_PUBLIC_KEY_LENGTH          32
#define AES_SECRET_PART_LEN           16

#define TESTER_SQL_HOST_NAME_LEN      128
#define TRACE_MAX_DEPTH               30
#define SESSION_PASS_MAX_LEN          48

#define MODULE_BOD_NAME               "BOD"
#define MODULE_CHG_NAME               "CHG"

//#define CHECK_OK_HAS_ADDINFO          0x80000000
#define TRACE_DELAYMS_NO_ROUTE        0x8000
#define MAX_DELAY_MS                  0x7FFF
#define DELAY_NO_REPLY                0xFFFF

#define SOCK_RC_TIMEOUT               -1
#define SOCK_RC_AUTH_FAILURE          -2
#define SOCK_RC_WRONG_SIZES           -3
#define SOCK_RC_NOT_ENOUGH_MEMORY     -4
#define SOCK_RC_CRC_ERROR             -5
#define SOCK_RC_DECOMPRESS_FAILED     -6
#define SOCK_RC_INTEGRITY_FAILED      -7
#define SOCK_RC_SQL_ERROR             -8
#define SOCK_RC_UNKNOWN_REQUEST       -9
#define SOCK_RC_TOO_MUCH_CONNECTIONS  -10
#define SOCK_RC_SERVER_SHUTTING_DOWN  -11
#define SOCK_RC_INCORRECT_DESTINATION -12

#define TESTER_FLAG_HAS_TRACE         1
#define TESTER_FLAG_CHECK_TASK        2
#define TESTER_FLAG_HAS_RAW_DATA      4
#define TESTER_FLAG_CHANGE_CFGVER     8

#define TESTER_TASK_FLAG_FORCE_RERESOLVING 1
#define TESTER_TASK_ONLINE_CHECKER         2

#define AGENT_FLAG_HAS_VALUE_CUR      1
#define AGENT_FLAG_HAS_VALUE_AVG      2
#define AGENT_FLAG_HAS_VALUE_MIN      4
#define AGENT_FLAG_HAS_VALUE_MAX      8
#define AGENT_FLAG_HAS_VALUE_ALL     (AGENT_FLAG_HAS_VALUE_CUR|AGENT_FLAG_HAS_VALUE_AVG|AGENT_FLAG_HAS_VALUE_MIN|AGENT_FLAG_HAS_VALUE_MAX)

#define TASK_FLAG_FORCE_RERESOLVE     0x00000001
#define TASK_FLAG_COLLECT_TRACE       0x00000002

#define SR_PROTOCOL_VERSION           0x0003

enum MODULE_TYPE {
    MODULE_PING=1,
    MODULE_TCP_PORT,        //2
    MODULE_HTTP,            //3
    MODULE_SMTP,            //4
    MODULE_FTP,             //5
    MODULE_DNS,             //6
    MODULE_POP,             //7
    MODULE_TELNET,          //8
    MODULE_SSH,             //9
    MODULE_CMS,             //10
    MODULE_TCP_PORT2=17,    //17
    MODULE_IMAP,            //18
    MODULE_HTTP_HEAD,       //19
    MODULE_NRPE,            //20
    MODULE_DBCONNECT,       //21
    MODULE_SAFE_BROWSING,   //22
    MODULE_WHOIS,           //23
    MODULE_HTTP_NAGIOS,     //24
    MODULE_DOMAIN,          //25
    MODULE_PORT_SCAN,       //26
    MODULE_READ_OBJECTS_CFG=1000,
    MODULE_CHECK_TASKS,
    MODULE_UPDATE_TESTER_DATA, // Payload==new binary
    MODULE_SET_SOCK_TIMEOUT,   // Payload==uint32_t milliseconds
    MODULE_AGENT,
    MODULE_SET_BASES_IPS       // Payload=TESTER_CONNECT_IPS, response=1
};

const char* taskNameById(int id);

inline bool verifyPlugin(int type) {
    if(type >= 1 && type <= 10) {
        return true;
    } else if(type >= 17 && type <= 26) {
        return true;
    }
    return false;
}

#define CONTEXT_MAXIMAL_UNCMPR_BUF_SZ   (2*1024*1024)

#define MODULE_LAST_MODULE MODULE_HTTP_NAGIOS

#pragma pack(push, 1)

struct sChgRecord {
    // id объекта мониторинга
    uint32_t    LObjId;

    // тип модуля тестирования (пинг-порт-хттп-...)
    uint16_t ModType;

    // Индекс проверки
    uint32_t PObjId;

    // Индекс клиента
    uint32_t ClientId;

    // дата проверки, unixtime utc
    uint32_t CheckDt;

    // ип тестировавшегося объекта
    uint32_t IP;

    // Код результата проверки. При успешной проверке - 0
    uint8_t CheckRes;

    // время получения результата в мс, или 0xFFFF если ответ не был получен
    uint16_t DelayMS;

    // ИП сообщивший о недоступности/проблемах или 0
    uint32_t ProblemIP;

    // icmp.type проблемы от хоста ProblemIP
    uint16_t ProblemICMP_Type;

    // icmp.code проблемы от хоста ProblemIP
    uint16_t ProblemICMP_Code;

    // Нормированный результат проверки. 0-Alarm, 1 - Ok, 2 - warning
    uint8_t CheckStatus;

    // TESTER_FLAG_*
    uint8_t Flags;

    // Номер генерации конфига. Инкрементируется в пределах 0-249
    uint8_t CFGVer;

    uint16_t Port;

    // Индекс базы, с которой работает тестер
    uint16_t baseIndex;

} __attribute__((aligned(1),packed));


struct sChgRecordA {
    uint32_t    LObjId;
    uint32_t    CheckDt;
    uint8_t     CheckStatus;
    int32_t     ErrorCode;
    uint32_t    ModuleId;
    uint32_t    AgentId;
    // SenseInfo:
    int64_t     ValueCur;
    int64_t     ValueAvg;
    int64_t     ValueMin;
    int64_t     ValueMax;
    uint8_t     Flags; // AGENT_FLAG_HAS_VALUE_*
} __attribute__((aligned(1),packed));

typedef struct sTesterConfigRecord {
    // Индекс проверки
    uint32_t LObjId;

    // тип модуля тестирования (пинг-порт-хттп-...)
    uint16_t ModType;

    // порт проверки
    uint16_t Port;

    // периодичность проверки объекта в секундах
    uint32_t CheckPeriod;

    // периодичность проверки ИП-адреса в секундах,
    // или 0, если проверку делать не нужно
    uint32_t ResolvePeriod;

    // IP объекта тестирования
    uint32_t IP;

    // дата ближайшей проверки. unixtime utc
    uint32_t NextCheckDt;

    // id объекта тестирования
    uint32_t PObjId;

    // Индекс клиента
    uint32_t ClientId;

    // LObjId предыдущего хоста в folded-цепочке или 0
    uint32_t FoldedNext;

    // LObjId следующего хоста в folded-цепочке или 0
    uint32_t FoldedPrev;

    // Лимит проверки результата I
    uint32_t TimeOut;

    // Лимит проверки результата W
    int64_t LimWarn;

    // Лимит проверки результата A
    int64_t LimAlrt;

    // Направление проверки лимитов
    // 0 Значение наблюдаемой величины выше
    // 1 Значение наблюдаемой величины ниже
    // 2 Значение наблюдаемой величины равно
    uint8_t LimDir;

    // номер генерации конфига.
    // Должен подставляться в отчеты
    uint8_t CFGVer;

    // Флаги
    uint8_t Flags;

    uint8_t HostNameLen;

    // Длина доп-конфига.
    uint16_t ConfigLen;

    // Индекс базы, с которой работает тестер
    uint16_t baseIndex;

    size_t GetAddInfoSz();

    size_t GetSize();

    sTesterConfigRecord* GetNext();

    char* GetHostNamePtr();

    bool CmpHostName(char *CmpStr);

    bool CmpiHostName(char *CmpStr);

    void CpyHostName(char *dst);

} __attribute__((aligned(1),packed)) TESTER_CFG_RECORD, *PTESTER_CFG_RECORD;

//-------------------------------------------------------------------
// Container entity
//

typedef std::array<uint8_t, MD5_DIGEST_LENGTH> md5_type;

class TesterConfigRecord :
        public sTesterConfigRecord {
public:
    TesterConfigRecord();
    TesterConfigRecord(const sTesterConfigRecord* record);
    TesterConfigRecord(const TesterConfigRecord& record);

    // Assign config from base struct pointer
    TesterConfigRecord& operator=(const TesterConfigRecord* record);

    // Debug output print
    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream)  {

        aStream << "LObjId="     << LObjId << "\t";
//        aStream << "PObjId="     << PObjId << "\t";
        aStream << "baseIndex="  << std::dec << baseIndex << "\t";
//        aStream << "FoldedPrev=" << FoldedPrev << "\t";
//        aStream << "FoldedNext=" << FoldedNext << "\t";
        aStream << "ModType="    << ModType << "\t";
        aStream << "NetString="  << _config << "\t";

        if(IP != 0) {
            aStream << "IP=" << std::dec
                    << (int)(uint8_t)(IP>>8) << "." << (int)(uint8_t)(IP>>4) << "."
                    << (int)(uint8_t)(IP>>2) << "." << (int)(uint8_t)(IP) << "\t";
        } else {
            aStream << "<The record has not IP!!>" << std::endl;
        }

        if((int)(HostNameLen)) {
            aStream << "HostNameLen=" << std::dec << (int)(HostNameLen) << "\t";
            aStream << "HostName=" << _hostName << "\t";
        }

        if(ConfigLen) {
            aStream << "ConfigLen=" << std::dec << ConfigLen << "\t";
            aStream << "NetString=" << _config;
        }
    }

    // Get string host name
    std::string& hostName();

    // Get netstring
    std::string& config();

private:
    // IP address or domain name in string value
    std::string _hostName;

    // Netstring config
    std::string _config;

    // Init strings parameters for saving it
    void initStrings_(const sTesterConfigRecord* record);
};

typedef struct _Tester_Check_Record {
    uint32_t    LObjId;
    uint32_t    ModuleId;
    uint16_t    Port;
    uint32_t    IP;
    uint32_t    CheckDt;
    uint32_t    PObjId;
    uint32_t    ClientId;
    uint32_t    TimeOut;
    int64_t     LimWarn;
    int64_t     LimAlrt;
    uint8_t     LimDir;
    uint8_t     CFGVer;
    uint8_t     Flags;
    uint8_t     HostNameLen;
    uint16_t    ConfigLen;
    //  struct {
    //    uint32_t            HostNameLen:8;
    //    uint32_t            ConfigLen:24;
    //  };

    size_t GetAddInfoSz() {
        return this->HostNameLen+this->ConfigLen;
    }

    size_t GetSize() {
        return sizeof(*this)+this->GetAddInfoSz();
    }

    _Tester_Check_Record* GetNext() {
        return (_Tester_Check_Record *) (((char *) &this[1])+this->GetAddInfoSz());
    }

    char* GetHostNamePtr() {
        return (char *) (&this[1]);
    }

    bool CmpHostName(char *CmpStr) {
        return strlen(CmpStr) == this->HostNameLen &&
                !memcmp(CmpStr,this->GetHostNamePtr(),this->HostNameLen);
    }

    bool CmpiHostName(char *CmpStr) {
        return strlen(CmpStr)==this->HostNameLen&&!strcasecmp(CmpStr,this->GetHostNamePtr());
    }

    void CpyHostName(char *dst) {
        memcpy(dst, this->GetHostNamePtr(), this->HostNameLen); dst[this->HostNameLen]=0;
    }
} TESTER_CHECK_RECORD, *PTESTER_CHECK_RECORD;

typedef struct _Tester_Connect_IPs {
    union {
        struct { uint8_t  BODip1,BODip2,BODip3,BODip4;};
        uint32_t    BODip32;
    };
    uint16_t    BODport;
    union {
        struct { uint8_t  CHGip1,CHGip2,CHGip3,CHGip4;};
        uint32_t    CHGip32;
    };
    uint16_t    CHGport;
} TESTER_CONNECT_IPS, *PTESTER_CONNECT_IPS;

typedef struct _st_sizes {
    uint32_t    CmprSize;
    uint32_t    UncmprSize;
    uint32_t    crc;

    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream) {
        aStream << "ST_SIZES: CmprSize = " << CmprSize
                << ", UncmprSize = " << UncmprSize
                << ", crc = " << crc;
    }

} ST_SIZES, *PST_SIZES;

typedef struct _st_t_hdr {
    uint32_t    TesterId;
    uint32_t    ReqType; // MODULE_TYPE

    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream) {
        aStream << "ST_T_HDR: TesterId = " << TesterId
                << ", ReqType = " << ReqType;
    }

} ST_T_HDR, *PST_T_HDR;

typedef struct _Tester_Cfg_AddData {
    uint16_t    ProtocolVersion; // SQLReceiver<->Tester protocol compatibility
    uint32_t    ServerTime;      // Server's localtime.
    uint32_t    CfgRereadPeriod; // Tester reread config period
    uint32_t    BODMaxAge;       // Operdata DB cache MaxAge
    uint32_t    CHGMaxAge;       // CHG DB cache MaxAge

    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream) {
        aStream << "TESTER_CFG_ADDDATA: ProtocolVersion = " << ProtocolVersion
                << ", ServerTime = " << ServerTime
                << ", CfgRereadPeriod = " << CfgRereadPeriod
                << ", BODMaxAge = " << BODMaxAge
                << ", CHGMaxAge = " << CHGMaxAge;
    }

} TESTER_CFG_ADDDATA, *PTESTER_CFG_ADDDATA;


typedef unsigned char CURVE25519_KEY[DH_PUBLIC_KEY_LENGTH];

typedef  struct _session_key {
    CURVE25519_KEY ClientPubKey;
    char    garbage[AES_BLOCK_SIZE];
    char    pass[SESSION_PASS_MAX_LEN];
} SESSION_KEY, *PSESSION_KEY;

typedef  struct _session_subkey {
    char    garbage[AES_BLOCK_SIZE];
    char    pass[SESSION_PASS_MAX_LEN];
} SESSION_SUBKEY, *PSESSION_SUBKEY;


typedef struct _trace_host {
    union {
        struct { uint8_t  ip1,ip2,ip3,ip4;};
        uint32_t          ip32;
    };
    uint16_t    delay;
} TRACE_HOST, *PTRACE_HOST;

typedef struct _traceroute {
    uint8_t     depth;
    TRACE_HOST  hop[TRACE_MAX_DEPTH];
} TRACEROUTE, *PTRACEROUTE;

typedef struct _mon_pkt {
    ST_SIZES    sizes;
    ST_T_HDR    hdr;
} MON_PKT, *PMON_PKT;

#define TESTER_FLAGS_ONLINE         0x00000001
#define TESTER_FLAGS_NEED_FOLDING   0x00000010
#define TESTER_FLAGS_NEED_SORTING   0x00000100
#define TESTER_FLAGS_CAN_CHECK      0x00001000
#define TESTER_FLAGS_CAN_RESOLVE    0x00010000
#define TESTER_FLAGS_HAS_RAW_DATA   0x00100000

typedef struct _tester_info {
    uint8_t     Flags;             // 0x01 == Online/Offline
    // 0x02 == Need calculated folding info
    // 0x04 == Need LObjId-sorted data-array
    // 0x08 == Can do check tasks
    // 0x10 == Can do dns resolving
    uint8_t     Load;              // Tester load 0-100
    uint8_t     SoftwareInfoLen;
    //  char           SoftwareInfo[255]; // Tester software signature
    size_t GetAddInfoSz() {
        return this->SoftwareInfoLen;
    }

    size_t GetSize() {
        return sizeof(*this)+this->GetAddInfoSz();
    }
} TESTER_INFO, *PTESTER_INFO;


inline const char* rc_error(int32_t error) {
    switch (error) {
    case -1:  return "SOCK_RC_TIMEOUT";
    case -2:  return "SOCK_RC_AUTH_FAILURE";
    case -3:  return "SOCK_RC_WRONG_SIZES";
    case -4:  return "SOCK_RC_NOT_ENOUGH_MEMORY";
    case -5:  return "SOCK_RC_CRC_ERROR";
    case -6:  return "SOCK_RC_DECOMPRESS_FAILED";
    case -7:  return "SOCK_RC_SQL_ERROR";
    case -8:  return "SOCK_RC_SQL_ERROR";
    case -9:  return "SOCK_RC_UNKNOWN_REQUEST";
    case -10: return "SOCK_RC_TOO_MUCH_CONNECTIONS";
    case -11: return "SOCK_RC_SERVER_SHUTTING_DOWN";
    case -12: return "SOCK_RC_INCORRECT_DESTINATION";
    default:
        return "Unknown";
    }
}

#pragma pack(pop)


