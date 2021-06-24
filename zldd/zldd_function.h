/**
 * zldd multiplatform dynamic library loader.
 *
 * @copyright 2017 Eduard Pozdnyakov
 */

#ifndef __ICE_FUNCTIONS_H__
#define __ICE_FUNCTIONS_H__

#include "zldd_library.h"
#include "zldd_exception.h"
#include <string>
#include <sstream>

namespace zldd
{
template <class Signature>
class Function;
}

template <class Signature>
class zldd::Function
{
public:
    Function(zldd::Library* library, std::string name)
        : m_name(name)
    {
        m_lib = library;
        if (library == NULL)
        {
            std::stringstream ss;
            ss << "Library is NULL, can't call function: '" <<
                name << "'";
            throw zldd::Exception(ss.str());
        }
        m_libname = library->name();
#if (defined(_WIN32) || defined(__WIN32__))
        m_func = reinterpret_cast<Signature*>(
            GetProcAddress(library->_library(), name.c_str()));
        if (m_func == NULL)
        {
            std::stringstream err;
            err << "Failed to Retrieve address of function '" << name <<
                "': Windows Error #" << GetLastError() << " for library '" <<
                library->name() << "'";
            throw zldd::Exception(err.str());
        }
#else
        m_func = reinterpret_cast<Signature*>(
            dlsym(library->_library(), name.c_str()));
        if (m_func == NULL)
        {
            std::stringstream err;
            err << "Failed to Retrieve address of function '" << name <<
                "': " << dlerror() << " for library '" <<
                library->name() << "'";
            throw zldd::Exception(err.str());
        }
#endif
    }
    Function(zldd::Library* library, unsigned int ordinal)
        : m_name(__ordinalToString(ordinal))
    {
        /*
        std::stringstream temp;
        temp << "Ordinal " << ordinal;
        m_name = temp.str().c_str();
        */
        // Make sure the ordinal fits inside the lower word (16-bit)
        if (ordinal > 0xFFFF)
        {
            std::stringstream ss;
            ss << "Can't call function: '" <<
                m_name << "' due to ordinal number being over 16-bit limitation";
            throw zldd::Exception(ss.str());
        }
        m_lib = library;
        if (library == NULL)
        {
            std::stringstream ss;
            ss << "Library is NULL, can't call function: '" <<
                m_name << "'";
            throw zldd::Exception(ss.str());
        }
        m_libname = library->name();
#if (defined(_WIN32) || defined(__WIN32__))
        m_func = reinterpret_cast<Signature*>(
            GetProcAddress(library->_library(), MAKEINTRESOURCEA(ordinal)));
        if (m_func == NULL)
        {
            std::stringstream err;
            err << "Failed to Retrieve address of function '" << m_name <<
                "': Windows Error #" << GetLastError() << " for library '" <<
                library->name() << "'";
            throw zldd::Exception(err.str());
        }
#else
        m_func = reinterpret_cast<Signature*>(
            dlsym(library->_library(), (const char*)ordinal));
        if (m_func == NULL)
        {
            std::stringstream err;
            err << "Failed to Retrieve address of function '" << m_name <<
                "': " << dlerror() << " for library '" <<
                library->name() << "'";
            throw zldd::Exception(err.str());
        }
#endif
    }
    operator Signature *() const
    {
        if (m_func == NULL)
        {
            std::stringstream ss;
            if (!m_lib)
            {
                ss << "Function address '" << m_name + "' isn't resolved for unloaded library: '" <<
                    m_libname << "'";
            }
            else
            {
                ss << "Function address '" << m_name + "' isn't resolved for library: '" <<
                    m_lib->name() << "'";
            }
            throw zldd::Exception(ss.str());
        }
        return m_func;
    }
    bool isValid() const { return m_func != NULL; }
    std::string name() const { return m_name; }

protected:
    Signature* m_func;
    const std::string m_name;
    std::string m_libname;

private:
    // This gets around only being able to initialize const objects in constructor initializer lists
    static std::string __ordinalToString(unsigned int x)
    {
        std::stringstream ss;
        ss << "Ordinal #" << x;
        return ss.str();
    }

	zldd::Library* m_lib;
};

#endif // zldd_function.h
