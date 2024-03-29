/**
 * ldd multiplatform dynamic library loader.
 *
 * @copyright 2017 Eduard Pozdnyakov
 */

#ifndef __ICE_LIBRARY_H__
#define __ICE_LIBRARY_H__

#if (defined(_WIN32) || defined(__WIN32__))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#define HMODULE void*
#endif

#include <string>

namespace ldd
{
class Library;
};

class ldd::Library
{
    HMODULE m_lib;
    std::string m_name;

public:
    Library(std::string name);
    ~Library();
    bool isLoaded() const;
    std::string name() const { return m_name; }
    
    std::string getPath(bool* okay=NULL);

    HMODULE const& _library() const;
};

#endif // ldd_library.h
