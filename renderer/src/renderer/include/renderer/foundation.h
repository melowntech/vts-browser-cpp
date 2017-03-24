#ifndef FOUNDATION_H_kjebnj
#define FOUNDATION_H_kjebnj

#include <cstdint>

namespace melown
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
#define MELOWN_API_EXPORT _declspec(dllexport)
#define MELOWN_API_IMPORT _declspec(dllimport)
#define MELOWN_THREAD_LOCAL_STORAGE __declspec(thread)
#else
#define MELOWN_API_EXPORT __attribute__((visibility ("default")))
#define MELOWN_API_IMPORT __attribute__((visibility ("default")))
#define MELOWN_THREAD_LOCAL_STORAGE __thread
#endif

#ifdef MELOWN_RENDERER_BUILD_SHARED
#define MELOWN_API MELOWN_API_EXPORT
#else
#define MELOWN_API MELOWN_API_IMPORT
#endif

} // namespace melown

#endif

