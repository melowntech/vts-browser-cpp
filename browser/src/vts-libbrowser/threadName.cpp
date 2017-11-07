
#ifdef __linux__

#include <sys/prctl.h>

void setThreadName(const char *name)
{
#ifndef NDEBUG
    prctl(PR_SET_NAME,name,0,0,0);
#else
    (void)name;
#endif
}

#else

void setThreadName(const char *name)
{}

#endif

