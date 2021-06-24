/**
 * zldd multiplatform dynamic library loader.
 *
 * @copyright 2017 Eduard Pozdnyakov
 */

#ifndef __ICE_EXCEPTION_H__
#define __ICE_EXCEPTION_H__

#include <string>
#include <stdexcept>

namespace zldd
{
class Exception;
};

class zldd::Exception : std::exception
{
    const std::string m_msg;
public:
    Exception(const std::string msg)
        : m_msg(msg) {}
    ~Exception() throw() {}
    const char* what() const throw()
    { return this->m_msg.c_str(); }
    std::string const whatString() const
    { return this->m_msg; }
};

#endif // zldd_exception.h
