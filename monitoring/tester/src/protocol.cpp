#include <vector>

#include "hex_dump.h"
#include "protocol.h"
#include "log.h"

static const char* _tag = "[PROTO] ";

//------------------------------------------------------------
const char* taskNameById(int id) {
    switch(id) {
    case MODULE_PING:
        return "PING";
    case MODULE_TCP_PORT:
        return "TCP_PORT";
    case MODULE_HTTP:
        return "HTTP";
    case MODULE_SMTP:
        return "SMTP";
    case MODULE_FTP:
        return "FTP";
    case MODULE_DNS:
        return "DNS";
    case MODULE_POP:
        return "POP";
    case MODULE_TELNET:
        return "TELNET";
    case MODULE_SSH:
        return "SSH";
    case MODULE_CMS:
        return "CMS";
    case MODULE_TCP_PORT2:
        return "PORT2";
    case MODULE_IMAP:
        return "IMAP";
    case MODULE_HTTP_HEAD:
        return "HTTP_HEAD";
    case MODULE_NRPE:
        return "NRPE";
    case MODULE_DBCONNECT:
        return "DBCONNECT";
    case MODULE_WHOIS:
        return "WHOIS";
    case MODULE_HTTP_NAGIOS:
        return "HTTP_NAGIOS";
    case MODULE_DOMAIN:
        return "DOMAIN";
    case MODULE_PORT_SCAN:
        return "PORT_SCAN";
    }
    return "OLOLO";
}

//------------------------------------------------------------
// sTesterConfigRecord
//

size_t sTesterConfigRecord::GetAddInfoSz() {
    return (int)(HostNameLen) + ConfigLen;
}

size_t sTesterConfigRecord::GetSize() {
    return sizeof(*this) + this->GetAddInfoSz();
}

sTesterConfigRecord* sTesterConfigRecord::GetNext() {
    return (sTesterConfigRecord*)((((uint8_t*)&this[1]) + GetAddInfoSz()));
}

char* sTesterConfigRecord::GetHostNamePtr() {
    return (char *) (&this[1]);
}

bool sTesterConfigRecord::CmpHostName(char *CmpStr) {
    return strlen(CmpStr) == HostNameLen&&!memcmp(CmpStr, GetHostNamePtr(), HostNameLen);
}

bool sTesterConfigRecord::CmpiHostName(char *CmpStr) {
    return strlen(CmpStr) == HostNameLen&&!strcasecmp(CmpStr, GetHostNamePtr());
}

void sTesterConfigRecord::CpyHostName(char *dst) {
    memcpy(dst, GetHostNamePtr(), HostNameLen); dst[HostNameLen] = 0;
}

//------------------------------------------------------------
// TesterConfigRecord
//

TesterConfigRecord::TesterConfigRecord()
{}

TesterConfigRecord::TesterConfigRecord(const sTesterConfigRecord* record) {
    *((sTesterConfigRecord*)this) = *record;
    initStrings_(record);
}

std::string& TesterConfigRecord::config() {
    return _config;
}

std::string& TesterConfigRecord::hostName() {
    return _hostName;
}

TesterConfigRecord::TesterConfigRecord(const TesterConfigRecord& record) {
    *this = (sTesterConfigRecord*)&record;
    this->_hostName = record._hostName;
    this->_config = record._config;
}

/**
 * @brief TesterConfigRecord::operator = Assign config from base struct pointer
 * @param record
 * @return
 */
TesterConfigRecord& TesterConfigRecord::operator=(const TesterConfigRecord* record) {
    *((sTesterConfigRecord*)this) = *record;
    this->_hostName = record->_hostName;
    this->_config = record->_config;
    return *this;
}

/**
 * @brief TesterConfigRecord::initStrings_ - Init strings parameters for saving it
 * @param record
 */
void TesterConfigRecord::initStrings_(const sTesterConfigRecord *record) {
    if((int)(HostNameLen)) {
        std::vector<uint8_t> hostName((int)(HostNameLen));
        std::copy((uint8_t*)record + sizeof(sTesterConfigRecord),
                  (uint8_t*)record + sizeof(sTesterConfigRecord) + (int)(HostNameLen), hostName.begin());
        hostName.push_back('\x0');
        _hostName = std::string(hostName.begin(), hostName.end());
    }
    if(ConfigLen) {
        std::vector<uint8_t> netString(ConfigLen);
        std::copy((uint8_t*)record + sizeof(sTesterConfigRecord) + (int)(HostNameLen),
                  (uint8_t*)record + sizeof(sTesterConfigRecord) + (int)(HostNameLen) + ConfigLen, netString.begin());
        netString.push_back('\x0');
        _config = std::string(netString.begin(), netString.end());
    }
}



