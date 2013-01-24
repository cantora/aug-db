#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

int util_usleep(int secs, int usecs) {
	struct timeval amt;
	amt.tv_sec = secs;
	amt.tv_usec = usecs;
	
	if(select(-1, NULL, NULL, NULL, &amt) != 0)
		return -1;

	return 0;
}