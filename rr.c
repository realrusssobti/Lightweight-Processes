#include "lwp.h"
#include "rr.h"
#include <stddef.h>

thread head = NULL;
thread tail = NULL;
int qlen = 0;

void rr_init(void){
    head = NULL;
    tail = NULL;
    qlen = 0;
}

void rr_shutdown(void){
    head = NULL;
    tail = NULL;
    qlen = 0;
}


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
	qlen++;
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
				qlen--;
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
        
int rr_qlen(){
	return qlen;
}
