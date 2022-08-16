#ifndef __ICE_LIBRARY_MANAGER_H__
#define __ICE_LIBRARY_MANAGER_H__

#include "ldd_library.h"
#include <map>
#include <string>
#include <vector>

namespace ldd
{
    class LibraryManager;
};

class ldd::LibraryManager
{
public:
    LibraryManager();
    ~LibraryManager();
    
    void add(std::string name, std::string path, bool replace=false);
    void remove(std::string name);
    void remove(ldd::Library& library);

    std::vector<std::string> getLibraryNames() const;
    
    ldd::Library* operator[](std::string name);
    ldd::Library* get(std::string name) { return m_libs[name]; }

    static LibraryManager& instance();

    bool exists(std::string name) const;
    
private:
    typedef std::map<std::string, ldd::Library*> Libraries;
    Libraries m_libs;
};

#endif // __ICE_LIBRARY_MANAGER_H__
