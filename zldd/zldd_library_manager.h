#ifndef __ICE_LIBRARY_MANAGER_H__
#define __ICE_LIBRARY_MANAGER_H__

#include "zldd_library.h"
#include <map>
#include <string>
#include <vector>

namespace zldd
{
    class LibraryManager;
};

class zldd::LibraryManager
{
public:
    LibraryManager();
    ~LibraryManager();
    
    void add(std::string name, std::string path, bool replace=false);
    void remove(std::string name);
    void remove(zldd::Library& library);

    std::vector<std::string> getLibraryNames() const;
    
    zldd::Library* operator[](std::string name);
    zldd::Library* get(std::string name) { return m_libs[name]; }

    static LibraryManager& instance();

    bool exists(std::string name) const;
    
private:
    typedef std::map<std::string, zldd::Library*> Libraries;
    Libraries m_libs;
};

#endif // __ICE_LIBRARY_MANAGER_H__