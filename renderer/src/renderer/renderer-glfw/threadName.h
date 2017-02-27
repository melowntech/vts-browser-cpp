
#include <sys/prctl.h>
inline void setThreadName(const char *name)
{
	prctl(PR_SET_NAME,name,0,0,0);
}

