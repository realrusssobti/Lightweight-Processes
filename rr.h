#ifndef RRH
#define RRH
#include "lwp.h"

void rr_init(void);
void rr_shutdown(void);
void rr_admit(thread new);
void rr_remove(thread victim);
thread rr_next(void);
int rr_qlen(void);
#endif
