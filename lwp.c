//
// Created by Russ Sobti and Diana Koralski on 2/15/24.
//

#include "lwp.h"
#include "rr.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STACK_SIZE (1024*1024*8) // 8 MB
#define MAX_LWPS 32

tid_t tid_counter = 1;
thread current = NULL;
scheduler this_scheduler = NULL;
thread* threads[MAX_LWPS]; // Mapping tid to thread

typedef struct general_list{
        void   (*admit)(thread new, struct general_list *this_list);
        void   (*remove)(thread victim, struct general_list *this_list);
        thread head;
} list;

void w_admit(thread new, list *this_list);
void w_remove(thread victim, list *this_list);
void lwp_wrap(lwpfun fun, void *arg);

list wait_list = {w_admit, w_remove, NULL};
list blocked_list = {w_admit, w_remove, NULL};

struct scheduler rr_publish = {rr_init, rr_shutdown, rr_admit, rr_remove, rr_next, rr_qlen};
scheduler RoundRobin = &rr_publish;

void lwp_set_scheduler(scheduler x){
	this_scheduler = x;
	if (this_scheduler->init){
		this_scheduler->init();
	}
}

scheduler lwp_get_scheduler(void){
	return this_scheduler;	
}

void w_admit(thread new, list *this_list){
	if (new == NULL){
        	return;
    	}
    	if (this_list->head == NULL){
       		this_list->head = new;
		new->sched_one = NULL;
      	}
      	else{
          	thread current_thread = this_list->head;
          	while (current_thread->sched_one != NULL){
                   	current_thread = current_thread->sched_one;
           	}
             	current_thread->sched_one = new;
        	new->sched_one = NULL;                                         
	}
}

void w_remove(thread victim, list *this_list){
    if (victim == NULL){
        return;
    }
    thread previous_thread = NULL;
    thread current_thread = this_list->head;
    if (current_thread == NULL){
        return;
    }
    else {
        while (current_thread != NULL){
            if (current_thread->tid == victim->tid){
                if(previous_thread == NULL){
                    this_list->head = current_thread->sched_one;
                }
                else{
                    previous_thread->sched_one = current_thread->sched_one;
                }
                return;
            }
            previous_thread = current_thread;
            current_thread = current_thread->sched_one;
        }
    }
}

tid_t lwp_create(lwpfun function,void *argument){
   	if (this_scheduler == NULL){
		lwp_set_scheduler(RoundRobin);
	}
	
	thread new_thread = (thread)malloc(sizeof(context));
    unsigned long *base = (unsigned long *)mmap(NULL, STACK_SIZE,
	PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK,-1,0);
    if (base == MAP_FAILED){
        return NO_THREAD;
    }
	new_thread->stacksize = STACK_SIZE;
    unsigned long *TOS = base + (new_thread->stacksize/sizeof(unsigned long));
    new_thread->stack = base;
    TOS--;
    *TOS = (unsigned long )lwp_wrap;
    TOS--;
    new_thread->tid = tid_counter;
    tid_counter++;
    new_thread->state.rsp = (unsigned long) TOS;
    new_thread->state.rbp = (unsigned long) TOS;
    new_thread->state.fxsave=FPU_INIT;
    new_thread->state.rdi = (unsigned long) function;
    new_thread->state.rsi = (unsigned long) argument;
	new_thread->lib_one = NULL;
	new_thread->status = LWP_LIVE;
    this_scheduler->admit(new_thread);
	threads[new_thread->tid] = &new_thread;
	return new_thread->tid;
}


void lwp_start(void){
	if (this_scheduler == NULL){
        lwp_set_scheduler(RoundRobin);
    }
	thread new_thread = (thread)malloc(sizeof(context));
	new_thread->stack = NULL;
	new_thread->tid = 0;
	new_thread->lib_one = NULL;
	new_thread->status = LWP_LIVE;
	this_scheduler->admit(new_thread);
	current = new_thread;
	lwp_yield();
}

void lwp_exit(int status){
	unsigned int new_status = MKTERMSTAT(LWP_TERM, status);
	current->status = new_status;
	if(current->tid == 0){
		free(current);
		if (this_scheduler->shutdown){
			this_scheduler->shutdown();
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
		this_scheduler->remove(current);
		blocked_list.admit(current, &blocked_list);
		lwp_yield();
	}
	else{
		current->lib_one = wait_list.head;
		wait_list.remove(wait_list.head, &wait_list);
	}
	if(current->lib_one != NULL){
		thread dying_thread = current->lib_one;
		tid_t return_val = dying_thread->tid;
		if(status != NULL){
			*status = dying_thread->status;
		}
		if(munmap(dying_thread->stack, dying_thread->stacksize) == -1){
			printf("munmap failed\n");
		}
		free(dying_thread);
		current->lib_one = NULL;
		return return_val;		
	}
	else{
		printf("waiting thread does not have lib_one field\n");
		return NO_THREAD;
	}	
		
}

void lwp_yield(void){
	thread old = current;
	if(LWPTERMINATED(old->status)){
		this_scheduler->remove(old);
		thread current_blocked = blocked_list.head;
		int waitlist_flag = 1;
		if(current_blocked != NULL){
		while (current_blocked != NULL){
			if(current_blocked->lib_one == NULL){
				current_blocked->lib_one = old;
				blocked_list.remove(current_blocked, 
				&blocked_list);
				this_scheduler->admit(current_blocked);
				waitlist_flag = 0;
				break;
			}
			current_blocked = current_blocked->sched_one;
		}
		}
		if(waitlist_flag){
			wait_list.admit(old,&wait_list);	
		}						
	}
	current = this_scheduler->next();
	if(current == old){
		return;
	}
	swap_rfiles(&old->state, &current->state);
}

tid_t lwp_gettid(){
	return current->tid;
}

thread tid2thread(tid_t tid){
	if (tid >= 0 && tid < MAX_LWPS){
		return *threads[tid];
	}
	return NULL;	
}

void lwp_wrap(lwpfun fun, void *arg) {
	int rval;
	rval=fun(arg);
	lwp_exit(rval);
}

