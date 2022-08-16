#include <boost/filesystem.hpp>

#include "log.h"
#include "holder.h"

namespace fs = boost::filesystem;

static const char* _tag = "[HOLDER] ";

namespace tester {
/**
 * @brief Holder::Holder
 */
Holder::Holder() {}

/**
 * @brief Holder::loadPlugins
 * @param path
 */
void Holder::loadPlugins(const char *path) {
    if (fs::exists(path) && fs::is_directory(path)) {
        for (auto const & entry : fs::recursive_directory_iterator(path)) {
            if (fs::is_regular_file(entry) && entry.path().extension() == ".so") {
                LOG_TRACE << _tag << entry.path().filename();
                std::string p(std::string(path) + "/" + entry.path().filename().c_str());
                try {
                    ldd::Library* library = new ldd::Library(p.c_str());
                    ldd::Function<int()> type(library, "GetPluginType");
                    int i = type();
                    _plugins[i] = std::unique_ptr<ldd::Library>(library);
                } catch (ldd::Exception& ex) {
                    LOG_TRACE << _tag << "Error accept library " << entry.path().filename()
                              << ", error " << ex.whatString();
                }
            }
        }
    }
}

/**
 * @brief Holder::getTasksByPlugin - Get json object with plagins and them loads
 * @param h
 * @param d
 * @return
 */
std::string Holder::getTasksByPlugin() {
    auto tasks = Dispatcher::getNumberOfTask();
    std::string out = "[";
    if(tasks.size()) {
        for(auto it = tasks.begin(); it != tasks.end(); it++) {
            out += "{";
            out += "\"type\":" + std::to_string(it->first) + ",";
            out += "\"number\":" + std::to_string(it->second) + ",";
            if(_plugins.find(it->first) != _plugins.end()) {
                out += "\"enable\":\"true\"";
            } else {
                out += "\"enable\":\"false\"";
            }
            out += "},";
        }
        out.pop_back();
    }
    out += "]";
    return out;
}

ldd::Library* Holder::getLibraryByTask(int task) {
    if(_plugins.find(task) == _plugins.end()) {
        return nullptr;
    }
    return _plugins[task].get();
}

}
