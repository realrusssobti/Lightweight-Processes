#ifndef LISTH
#define LISTH
#include "lwp.h"

typedef struct list_struct{
	void (*enqueue)(thread new, struct list_struct *this_list);
	void (*dequeue)(thread victim, struct list_struct *this_list);
	thread head;
} list;

void enqueue(thread new, list *this_list);
void dequeue(thread victim, list *this_list);
#endif
