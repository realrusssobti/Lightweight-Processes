#include "lwp.h"

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
        tail->next = new;
        new->prev = tail;
        new->next = NULL;
        tail = new;
    }
}

void remove(thread victim) {
    thread current = tail;
    while (current && current->tid != victim->tid) {
        current = current->prev;
    }

    if (current == NULL || current->tid != victim->tid) {
        return; //victim not found
    }

    if (current->prev) {
        current->prev->next = current->next;
    } else {
        head = current->next;
        head->prev = NULL;
    }

    if (current->next) {
        current->next->prev = current->prev;
    } else {
        tail = current->prev;
        tail->next = NULL;
    }
    qlength--;
}

thread next(void) {
    thread current;

    if (qlength == 0) {
        return NULL;
    }

    current = head;
    head = head->next;
    if (head) {
        head->prev = NULL;
    } else {
        tail = NULL;
    }

    current->next = NULL;
    current->prev = NULL;

    qlength--;
    return current;
    //must call admit on thread to requeue in schedule after (not in) function
}

int qlen(){
    return qlength;
}
