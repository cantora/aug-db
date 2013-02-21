#include "util.h"
#include "api_calls.h"
#include "err.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

int util_usleep(int secs, int usecs) {
	struct timeval amt;
	amt.tv_sec = secs;
	amt.tv_usec = usecs;
	
	if(select(0, NULL, NULL, NULL, &amt) != 0)
		return -1;

	return 0;
}

int util_thread_init() {
	int status, old;

	if( (status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old)) != 0) {
		return status;
	}
	if( (status = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old)) != 0) {
		return status;
	}

	return 0;
}

int util_expand_path(const char *path, wordexp_t *exp) {
	int status;

	if( (status = wordexp(path, exp, WRDE_NOCMD)) != 0 ) {
		switch(status) {
		case WRDE_BADCHAR:
			aug_log("bad character in path: %s\n", path);
			break;
		case WRDE_CMDSUB:
			aug_log("command substitution in path: %s\n", path);
			break;
		case WRDE_SYNTAX:
			aug_log("syntax error in path: %s\n", path);
			break;
		default:
			aug_log("unknown error during configuration file path expansion\n");
		}
		return -1;
	}

	if(exp->we_wordc != 1) {
		if(exp->we_wordc == 0)
			aug_log("config file path %s did not expand to any words\n", path);
		else
			aug_log("config file path %s expanded to multiple words\n", path);

		wordfree(exp);
		return -1;
	}
	
	return 0;
}

char *util_tal_multiply(const void *ctx, const char *s, 
		const char *delim, size_t n) {
	const char **arr;
	char *result;
	size_t i, len, dlen;
		
	arr = malloc( sizeof(char *)*(n+1) );
	if(arr == NULL)
		err_panic(0, "memory error");

	for(i = 0; i < n; i++)
		arr[i] = s;
	arr[i] = NULL;

	result = strjoin(ctx, (char **)arr, delim);
	free(arr);

	if(n > 0) {
		dlen = strlen(delim);
		len = strlen(result);
		err_assert(len >= dlen);
		result[len - dlen] = '\0';
	}

	return result;
}

