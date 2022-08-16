#include "client/session.h"

#include "scope.h"
#include "log.h"

#include <boost/asio.hpp>

#include <string>

#include "client/session.h"
#include "scope.h"
#include "iplugin.h"
#include "json.h"
#include "protocol.h"
#include "plugin_api.h"

#include "log.h"

using boost::asio::ip::tcp;

static const char* _tag = "[FTP] ";

#if 0

TLogLevel Log::reportingLevel = DEBUG;

int main(int argc, char* argv[])
{
    try
    {
        if (int code = ftpList("test.rebex.net", "demo", "password", "/")) {
            return code;
        }
        return ftpRetrieve("test.rebex.net", "demo", "password", "/readme.txt");
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}

#endif

IPluginApi* pluginApi(PLUGIN_NAME) = nullptr;

namespace ftp {

class FtpExecutor: public Executor {
public:
    FtpExecutor(uint32_t id) : Executor(id) {}
    virtual ~FtpExecutor(){}

    template <class F> int FtpCommand(const char* addr, const char* user, const char* password, F func) {
        try {
            boost::asio::io_context io_context;

            // Get a list of endpoints corresponding to the server name.
            tcp::resolver resolver(io_context);
            tcp::resolver::results_type endpoints = resolver.resolve(addr, "ftp");

            // Try each endpoint until we successfully establish a connection.
            tcp::socket socket(io_context);
            boost::asio::connect(socket, endpoints);

            DEFER({ socket.close(); });

            ftp::client::Session session(resolver, socket);

            if (auto err = session.Connect(user, password)) {
                LOG_ERROR << _tag << "Cannot connect " << err.message();
                return 1;
            }

            LOG_DEBUG << _tag << "Connected";

            return func(session);
        } catch (const std::exception& ex) {
            LOG_ERROR << _tag << "Exception: " << ex.what();
            return 1;
        }
    }

    int ftpAuthenticate(const char* addr, const char* user, const char* password)
    {
        return FtpCommand(addr, user, password, [](ftp::client::Session& session) -> int {
            return 0;
        });
    }

    int ftpList(const char* addr, const char* user, const char* password, const char* path)
    {
        return FtpCommand(addr, user, password, [path](ftp::client::Session& session) -> int {
            size_t files_count = 0;
            if (auto err = session.ListDir(path, files_count)) {
                LOG_ERROR << _tag << "Cannot list directory " << err.message();
                return 1;
            }
            LOG_DEBUG << _tag << "Got " << files_count << " files";
            if (0 == files_count) {
                LOG_ERROR << _tag << "Empty response";
                return 1;
            }
            return 0;
        });
    }

    int ftpRetrieve(const char* addr, const char* user, const char* password, const char* path)
    {
        return FtpCommand(addr, user, password, [path](ftp::client::Session& session) -> int {
            size_t bytes_transferred = 0;
            if (auto err = session.Retrieve(path, bytes_transferred)) {
                LOG_ERROR << _tag << "Cannot retrieve file " << err.message();
                return 1;
            }
            LOG_DEBUG << _tag << "Transferred " << bytes_transferred << " bytes";
            if (0 == bytes_transferred) {
                LOG_ERROR << _tag << "Empty response";
                return 1;
            }
            return 0;
        });
    }

    virtual int32_t run() {

        // This need for save object data for using
        // after the thread will be detach

        auto savedHostName = _hostName;
        auto result = _result;

        try {
            // {"action":"auth","hostname":"176.74.219.29","login":"upload","passmode":"1","password":"Hdvkh8rgjl","path":""}
            // result = ftpList("176.74.219.29", "upload", "Hdvkh8rgjl", "");
                result = ftpList(_extData["hostname"].ToString().c_str(),
                        _extData["login"].ToString().c_str(),
                        _extData["password"].ToString().c_str(),
                        _extData["path"].ToString().c_str());
        }
        catch (std::exception& e) {
            LOG_ERROR << _tag << "Exception: " << e.what() << "\n";
        }

        return result;
    }
};

struct PluginApiInitializer {
    PluginApiInitializer() {
        static PluginApiClass<FtpExecutor> pluginApi;
        pluginApi(PLUGIN_NAME) = &pluginApi;
    }
};

} // namespace ftp

ftp::PluginApiInitializer initializer;















