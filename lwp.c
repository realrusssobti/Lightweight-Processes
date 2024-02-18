//
// Created by Russ Sobti and Diana Koralski on 2/15/24.
//

#include "lwp.h"
#include "rr.h"
#include "list.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define STACK_SIZE (1024*1024*8) // 8 MB
#define MAX_LWPS 32

tid_t last_tid = 0;
thread current = NULL;
thread* threads[MAX_LWPS]; // Mapping tid to thread

list wait_list = {enqueue, dequeue, NULL};
list blocked_list = {enqueue, dequeue, NULL};

struct scheduler rr_scheduler = {rr_init, rr_shutdown, rr_admit, rr_remove, rr_next, rr_qlen};
scheduler current_scheduler = &rr_scheduler;

void lwp_set_scheduler(scheduler x){
	current_scheduler = x;
	if (current_scheduler->init){
		current_scheduler->init();
	}
}

scheduler lwp_get_scheduler(void){
	return current_scheduler;	
}

void lwp_wrap(lwpfun fun, void *arg) {
	int rval = fun(arg);
	lwp_exit(rval);
}

tid_t lwp_create(lwpfun function,void *argument){
	thread new_thread = (thread)malloc(sizeof(context));
    unsigned long *base = (unsigned long *)mmap(NULL, STACK_SIZE,
		PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK,-1,0);
    if (base == MAP_FAILED){
        return NO_THREAD;
    }
	new_thread->stacksize = STACK_SIZE;
    unsigned long *offset = base + (new_thread->stacksize/sizeof(unsigned long));
    new_thread->stack = base;
    offset--;
    *offset = (unsigned long )lwp_wrap;
    offset--;
	last_tid++;
    new_thread->tid = last_tid;
    new_thread->state.rsp = (unsigned long) offset;
    new_thread->state.rbp = (unsigned long) offset;
    new_thread->state.fxsave = FPU_INIT;
    new_thread->state.rdi = (unsigned long) function;
    new_thread->state.rsi = (unsigned long) argument;
	new_thread->lib_one = NULL;
	new_thread->status = LWP_LIVE;
    current_scheduler->admit(new_thread);
	threads[new_thread->tid] = &new_thread;
	return new_thread->tid;
}

void lwp_start(void){
	thread main_thread = (thread)malloc(sizeof(context));
	main_thread->stack = NULL;
	main_thread->tid = 0;
	main_thread->lib_one = NULL;
	main_thread->status = LWP_LIVE;
	current_scheduler->admit(main_thread);
	current = main_thread;
	lwp_yield();
}

void lwp_exit(int status){
	unsigned int new_status = MKTERMSTAT(LWP_TERM, status);
	current->status = new_status;
	if(current->tid == 0){
		free(current);
		if (current_scheduler->shutdown){
			current_scheduler->shutdown();
		}
		return;		
	}
	lwp_yield();
}

tid_t lwp_wait(int *status){
	if (wait_list.head == NULL){
		if (rr_qlen() == 0){
		 	return NO_THREAD;
		}
		current_scheduler->remove(current);
		blocked_list.enqueue(current, &blocked_list);
		lwp_yield();
	}
	else{
		current->lib_one = wait_list.head;
		wait_list.dequeue(wait_list.head, &wait_list);
	}
	thread terminated = current->lib_one;
	if(terminated == NULL){
		return NO_THREAD;
	}
	if(status != NULL){
		*status = terminated->status;
	}
	if(munmap(terminated->stack, terminated->stacksize) == -1){
		printf("munmap error\n");
	}
	tid_t return_val = terminated->tid;
	free(terminated);
	current->lib_one = NULL;
	return return_val;
}

void lwp_yield(void){
	thread previous = current;
	if(LWPTERMINATED(previous->status)){
		//when the thread is terminated we need to put it in a blocked_list or wait_list
		current_scheduler->remove(previous);
		thread current_blocked = blocked_list.head;
		bool add_to_waitlist = true;
		while (current_blocked != NULL){
			if(current_blocked->lib_one == NULL){
				current_blocked->lib_one = previous;
				blocked_list.dequeue(current_blocked, 
				&blocked_list);
				current_scheduler->admit(current_blocked);
				add_to_waitlist = false;
				break;
			}
			current_blocked = current_blocked->sched_one;
		}
		if(add_to_waitlist){
			wait_list.enqueue(previous,&wait_list);	
		}						
	}
	current = current_scheduler->next();
	if(previous == current){ //the last thread is the main thread
		return;
	}
	swap_rfiles(&previous->state, &current->state);
}

tid_t lwp_gettid(){
	return current == NULL ? 0 : current->tid;
}

thread tid2thread(tid_t tid){
	if (tid >= 0 && tid < MAX_LWPS){
		return *threads[tid];
	}
	return NULL;	
}