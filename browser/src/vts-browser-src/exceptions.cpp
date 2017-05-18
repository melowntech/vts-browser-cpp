#include "include/vts-browser/exceptions.hpp"

namespace vts
{

MapConfigException::MapConfigException(const std::string &what_arg) :
    std::runtime_error(what_arg)
{}

} // namespace vts
