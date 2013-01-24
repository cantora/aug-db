#ifndef AUG_DB_ERR_DISPATCH_H
#define AUG_DB_ERR_DISPATCH_H

/* returns a non zero error number on failure */
int err_dispatch_init(void (*cleanup)(int error) );
/* returns a non zero error number on failure */
int err_dispatch_free();
/* dispatch error handling thread */
void err_watchdog_signal(int error);

#endif /* AUG_DB_ERR_DISPATCH_H */
