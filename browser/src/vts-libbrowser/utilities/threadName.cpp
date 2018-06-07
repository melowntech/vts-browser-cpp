
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#include <windows.h>
#endif // _WIN32

#ifdef __linux__
#include <sys/prctl.h>
#endif


void setThreadName(const char *name)
{
    if (!name)
        return;

#ifdef _WIN32
    static const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
    struct THREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    };
#pragma pack(pop)
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = -1;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {}
#pragma warning(pop)
#endif

#ifdef __linux__
#ifndef NDEBUG
    prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
#endif
}
