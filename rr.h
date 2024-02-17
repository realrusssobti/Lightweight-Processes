#ifndef RRH
#define RRH
#include "lwp.h"

void init(void);
void shutdown(void);
void admit(thread new);
void remove(thread victim);
thread next(void);
int qlen(void);
#endif
