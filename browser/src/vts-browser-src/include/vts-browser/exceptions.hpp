#ifndef EXCEPTIONS_H_wgfrferg
#define EXCEPTIONS_H_wgfrferg

#include <string>
#include <stdexcept>

#include "foundation.hpp"

namespace vts
{

class VTS_API MapConfigException : public std::runtime_error
{
public:
    MapConfigException(const std::string &what_arg);
};

class VTS_API AuthException : public std::runtime_error
{
public:
    AuthException(const std::string &what_arg);
};

} // namespace vts

#endif
