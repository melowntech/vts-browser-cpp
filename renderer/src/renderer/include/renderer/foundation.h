#ifndef FOUNDATION_H_kjebnj
#define FOUNDATION_H_kjebnj

namespace melown
{
    typedef unsigned char uint8;
    typedef signed char sint8;
    typedef unsigned short uint16;
    typedef signed short sint16;
    typedef unsigned int uint32;
    typedef signed int sint32;
#ifdef _MSVC
    typedef unsigned long long uint64;
    typedef signed long long sint64;
#else
    typedef unsigned long uint64;
    typedef signed long sint64;
#endif

#ifdef _MSVC
#define MELOWN_API_EXPORT _declspec(dllexport)
#define MELOWN_API_IMPORT _declspec(dllimport)
#define MELOWN_THREAD_LOCAL_STORAGE __declspec(thread)
#else
#define MELOWN_API_EXPORT __attribute__ ((visibility ("default")))
#define MELOWN_API_IMPORT __attribute__ ((visibility ("default")))
#define MELOWN_THREAD_LOCAL_STORAGE __thread
#endif

#ifdef MELOWN_RENDERER_BUILD_SHARED
#define MELOWN_API MELOWN_API_EXPORT
#else
#define MELOWN_API MELOWN_API_IMPORT
#endif
}

#endif
