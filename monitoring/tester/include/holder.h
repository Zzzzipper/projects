#pragma once

#include <memory>

#include "log.h"
#include "ldd.h"
#include "dispatcher.h"

namespace  tester {

class Holder{
private:
    Holder();

public:
    Holder(const Holder&) = delete;
    Holder& operator=(const Holder &) = delete;
    Holder(Holder &&) = delete;
    Holder & operator=(Holder &&) = delete;

    static auto& instance(){
        static Holder tree;
        return tree;
    }

    // Load all comatible shared library from path
    void loadPlugins(const char* path);

    // Debug output print
    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream)  {
        aStream << "Loaded plugins: " << std::endl << std::endl;
        if(_plugins.size()) {
            auto iit = _plugins.begin();
            do {
                ldd::Function<int()> type(iit->second.get(), "GetPluginType");
                ldd::Function<const char* ()> name(iit->second.get(), "GetPluginName");
                aStream << "\tType: " << type() << ",\tName: " << name() << std::endl;
                iit++;
            } while(iit != _plugins.end());
        }
    }

    // Get pointer to library by task
    ldd::Library* getLibraryByTask(int task);

    // Get json object with plugins and them loads
    std::string getTasksByPlugin();

private:

    // Map of loaded plugins
    std::map<int, std::unique_ptr<ldd::Library>> _plugins;

};

}
