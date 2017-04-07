#ifndef FOUNDATION_H_kjebnj
#define FOUNDATION_H_kjebnj

#include <cstdint>

namespace vts
{

typedef std::uint8_t uint8;
typedef std::int8_t sint8;
typedef std::uint16_t uint16;
typedef std::int16_t sint16;
typedef std::uint32_t uint32;
typedef std::int32_t sint32;
typedef std::uint64_t uint64;
typedef std::int64_t sint64;

#ifdef _MSVC
#define VTS_API_EXPORT _declspec(dllexport)
#define VTS_API_IMPORT _declspec(dllimport)
#define VTS_THREAD_LOCAL_STORAGE __declspec(thread)
#else
#define VTS_API_EXPORT __attribute__((visibility ("default")))
#define VTS_API_IMPORT __attribute__((visibility ("default")))
#define VTS_THREAD_LOCAL_STORAGE __thread
#endif

#ifdef VTS_BROWSER_BUILD_SHARED
#define VTS_API VTS_API_EXPORT
#else
#define VTS_API VTS_API_IMPORT
#endif

} // namespace vts

#endif

