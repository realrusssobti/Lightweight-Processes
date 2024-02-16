#include "lwp.h"
#include <stddef.h>

thread head = NULL;
thread tail = NULL;
int qlength = 0;

void init(void){
    head = NULL;
    tail = NULL;
    qlength = 0;
}

void shutdown(void){
    head = NULL;
    tail = NULL;
    qlength = 0;
}

void admit(thread new) {
    qlength++;
    if (qlength == 1) { //add the first element
        head = new;
        tail = new;
    }
    else{
        tail->sched_one = new;
        new->sched_two = tail;
        new->sched_one = NULL;
        tail = new;
    }
}

void remove(thread victim) {
    thread current = tail;
    while (current && current->tid != victim->tid) {
        current = current->sched_two;
    }

    if (current == NULL || current->tid != victim->tid) {
        return; //victim not found
    }

    if (current->sched_two) {
        current->sched_two->sched_one = current->sched_one;
    } else {
        head = current->sched_one;
        head->sched_two = NULL;
    }

    if (current->sched_one) {
        current->sched_one->sched_two = current->sched_two;
    } else {
        tail = current->sched_two;
        tail->sched_one = NULL;
    }
    qlength--;
}

thread sched_one(void) {
    thread current;

    if (qlength == 0) {
        return NULL;
    }

    current = head;
    head = head->sched_one;
    if (head) {
        head->sched_two = NULL;
    } else {
        tail = NULL;
    }

    current->sched_one = NULL;
    current->sched_two = NULL;

    qlength--;
    return current;
    //must call admit on thread to requeue in schedule after (not in) function
}

int qlen(){
    return qlength;
}
