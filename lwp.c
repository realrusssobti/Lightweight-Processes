//
// Created by Russ Sobti and Diana Koralski on 2/15/24.
//

#include "lwp.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define STACK_SIZE (1024*1024*8) // 8 MB

tid_t tid_counter = 1;
thread running_thread = NULL;
scheduler this_scheduler = NULL;

thread head = NULL;

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


void rr_admit(thread new);
void rr_remove(thread victim);
thread rr_next(void);

void rr_admit(thread new){
    if (new == NULL){
        return;
    }
    if (head == NULL){
        head = new;
        new->sched_one = NULL;
    }
    else{
        thread current_thread = head;
        while (current_thread->sched_one != NULL){
            current_thread = current_thread->sched_one;
        }
        current_thread->sched_one = new;
        new->sched_one = NULL;
    }
}

void rr_remove(thread victim){
    if (victim == NULL){
        return;
    }
    thread previous_thread = NULL;
    thread current_thread = head;
    if (current_thread == NULL){
        return;
    }
    else {
        while (current_thread != NULL){
            if (current_thread->tid == victim->tid){
                if(previous_thread == NULL){
                    head = current_thread->sched_one;
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

thread rr_next(void){
        if(head == NULL){
                return NULL;
        }
        if(head->sched_one == NULL){
                return head;
        }
        thread current_thread = head;
        while (current_thread->sched_one != NULL){
                current_thread = current_thread->sched_one;
        }
        thread next = head;
        head = head->sched_one;
        current_thread->sched_one = next;
        next->sched_one = NULL;
        //printf("new head = %d, new tail = %d\n", head->tid, next->tid);
     	return next;
}
        
struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next};
scheduler RoundRobin = &rr_publish;

void lwp_set_scheduler(scheduler x){
	this_scheduler = x;
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
	//long page_size = sysconf(_SC_PAGE_SIZE);
	//printf("The page size is: %ld bytes\n", page_size);
	//struct rlimit limit;
	//int result = getrlimit(RLIMIT_STACK, &limit);
	//if (result == 0) {
        //	printf("Soft limit: %ld\n", limit.rlim_cur);
        //	printf("Hard limit: %ld\n", limit.rlim_max);
	//}	
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
    //swap_rfiles(NULL, &new_thread->state);
	return new_thread->tid;
}


void lwp_start(void){
	if (this_scheduler == NULL){
                lwp_set_scheduler(RoundRobin);
        }
	thread new_thread = (thread)malloc(sizeof(context));
	//swap_rfiles(&new_thread->state, NULL);
	new_thread->stack = NULL;
	new_thread->tid = 0;
	new_thread->lib_one = NULL;
	//new_thread->stacksize = ??
	new_thread->status = LWP_LIVE;
	this_scheduler->admit(new_thread);
	running_thread = new_thread;
	lwp_yield();
}

void lwp_exit(int status){
	unsigned int new_status = MKTERMSTAT(LWP_TERM, status);
	running_thread->status = new_status;
	//printf("thread new status = %u\n", new_status);
	if(running_thread->tid == 0){
		free(running_thread);
		return;		
	}
	//printf("running thread tid %d\n", running_thread->tid);	
	lwp_yield();
	
}

tid_t lwp_wait(int *status){
	if (wait_list.head == NULL){
		if(head->sched_one == NULL){
		 	return NO_THREAD;
		}
		//printf("adding thread %d to blocke
		this_scheduler->remove(running_thread);
		blocked_list.admit(running_thread, &blocked_list);
		lwp_yield();
	}
	else{
		running_thread->lib_one = wait_list.head;
		wait_list.remove(wait_list.head, &wait_list);
	}
	if(running_thread->lib_one != NULL){
		//printf("thread %d is killing thread %d\n", 
		thread dying_thread = running_thread->lib_one;
		tid_t return_val = dying_thread->tid;
		if(status != NULL){
			*status = dying_thread->status;
		}
		if(munmap(dying_thread->stack, dying_thread->stacksize) == -1){
			printf("munmap failed\n");
		}
		free(dying_thread);
		running_thread->lib_one = NULL;
		return return_val;		
	}
	else{
		printf("waiting thread does not have lib_one field\n");
		return NO_THREAD;
	}	
		
}

void print_queue(){
    thread current_thread = head;
    char str[256] = ""; // Make sure this is large enough for all list elemen
    char numBuffer[20]; // Buffer for converting numbers to strings

    while (current_thread != NULL){
        // Convert the thread id to a string and append to 'str'
      	snprintf(numBuffer, sizeof(numBuffer), "%d ", current_thread->status);
      	strcat(str, numBuffer);
                                 
     	// Move to the next thread
     	current_thread = current_thread->sched_one;	
	}
                                                         
    	// Print the concatenated string of thread ids
       	printf("%s\n", str);
      	}


void lwp_yield(void){
	//printf("running thread before next = %d\n", running_thread->tid);
	//fflush(stdout);
	thread old = running_thread;
	//print_queue();
	//printf("old thread = %d, new thread = %d\n", old->tid, new->tid);
	if(LWPTERMINATED(old->status)){
		this_scheduler->remove(old);
		thread current_blocked = blocked_list.head;
		int waitlist_flag = 1;
		if(current_blocked != NULL){
		while (current_blocked != NULL){
			if(current_blocked->lib_one == NULL){
				//printf("linking thread 
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
			//printf("adding thread %d to waitlist\n"
			wait_list.admit(old,&wait_list);	
		}						
	}
	running_thread = this_scheduler->next();
	if(running_thread == old){
		return;
	}
	//printf("running thread %d\n", running_thread->tid);
	//printf("bean bag cheezits\n");
	//print_queue();
	swap_rfiles(&old->state, &running_thread->state);
}

tid_t lwp_gettid(){
	return running_thread->tid;
}

thread tid2thread(tid_t tid){
	thread current_thread = head;
	while(current_thread != NULL){
		if (current_thread->tid == tid){
			return current_thread;
		}
		current_thread = current_thread->sched_one;		
	}
	return NULL;	
}

void lwp_wrap(lwpfun fun, void *arg) {
	int rval;
	//printf("argument = %ld\n", (long)arg);
	rval=fun(arg);
	lwp_exit(rval);
}

//int test(void *a){
//    int final = *((int *)a);
//	printf("final = %d", final);
//	fflush(stdout);
//    return final;
//}

//int main(){
//    int a = 5;
//    lwp_create(test, (void *)(&a), S_SIZE );
//    return 0;
//}
