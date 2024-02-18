#include "lwp.h"
#include "list.h"
#include <stddef.h>

void enqueue(thread new, list *this_list){
	if (new == NULL){
		return;
	}
	if (this_list->head == NULL){
		this_list->head = new;
		new->sched_one = NULL;
	}
	else{
		thread tail = this_list->head;
		while (tail->sched_one != NULL){
			tail = tail->sched_one;
		}
		tail->sched_one = new;
		new->sched_one = NULL;                                         
	}
}

void dequeue(thread victim, list *this_list){
    if (victim == NULL || this_list->head == NULL){
        return;
    }
    thread previous_thread = NULL;
    thread current_thread = this_list->head;
	while (current_thread != NULL){
		if (current_thread->tid == victim->tid){
			if(previous_thread == NULL){
				this_list->head = current_thread->sched_one;
			}
			else{
				previous_thread->sched_one = current_thread->sched_one;
			}
			break;
		}
		previous_thread = current_thread;
		current_thread = current_thread->sched_one;
	}
    
}
