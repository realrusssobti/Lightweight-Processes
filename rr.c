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
        tail = new;
        new->sched_one = NULL;
    }
    else{
        thread current_thread = head;
        tail->sched_one = new;
        new->sched_one = NULL;
        tail = new;
    }
	qlen++;
}

void rr_remove(thread victim){
    if (victim == NULL || head == NULL){
        return;
    }
    thread previous_thread = NULL;
    thread current_thread = head;
    while (current_thread != NULL){
        if (current_thread->tid == victim->tid){
            if(previous_thread == NULL){
                head = current_thread->sched_one;
                if (head == NULL){
                    tail = NULL;
                }
            }
            else{
                previous_thread->sched_one = current_thread->sched_one;
                if (tail == current_thread){
                    tail = previous_thread;
                }
            }
            qlen--;
            break;
        }
        previous_thread = current_thread;
        current_thread = current_thread->sched_one;
    }
}

thread rr_next(void){
        if(head == NULL){ // no elements
            return NULL;
        }
        if(head->sched_one == NULL){ // only 1 element
            return head;
        }
        thread new_tail = head;
        head = head->sched_one;
        tail->sched_one = new_tail;
        new_tail->sched_one = NULL;
        tail = new_tail;
     	return tail;
}
        
int rr_qlen(){
	return qlen;
}
