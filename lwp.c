//
// Created by Russ Sobti on 2/15/24.
//

// include some stuff
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "lwp.h"

// define some stuff
#define STACK_SIZE (1024*1024*8)
#define MAX_LWPS 32
#define READY 1
#define BLOCKED 2
#define DONE 3

static struct threadinfo_st threads[MAX_LWPS];
static tid_t current = NO_THREAD;
static scheduler SCHEDULER = NULL;
static void lwp_schedule();

extern tid_t lwp_create(lwpfun function, void *args) {
	// find a free thread slot
	int i;
	tid_t tid = NO_THREAD;
	for (i = 1; i < MAX_LWPS; i++) {
		if (threads[i].status == DONE) {
			tid = i;
			break;
		}
	}
	if (tid == NO_THREAD) {
		// print an error message
		fprintf(stderr, "No free threads available\n");
		return NO_THREAD;
	}
	unsigned long *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED) {
		// print an error message
		fprintf(stderr, "Failed to allocate stack. Uh - Oh!\n");
		return NO_THREAD;
	}
	// set up the stack
	struct threadinfo_st *thread = &threads[tid];
	thread->tid = tid;
	thread->stack = stack;
	thread->status = READY;
	thread->exited = 0;
	thread->lib_one = NULL;
	thread->lib_two = NULL;
	thread->sched_one = NULL;
	thread->sched_two = NULL;
	// set the argument and function
	thread->state.rdi = (unsigned long)args;
	thread->state.rip = (unsigned long)function;

	tid_t old = current;
	current = tid;
	lwp_schedule();
	// admit the new thread to the scheduler
	if(thread->status == READY) {
		SCHEDULER->admit(thread);
	}
	return tid;

}

extern void lwp_exit(int status) {
	struct threadinfo_st *thread = &threads[current];
	thread->exited = (thread)(unsigned long)(int)status;
	thread->status = DONE;
	tid_t old = current;
	current = SCHEDULER->next()->tid;
	lwp_schedule()
	// check if old thread needs to be admitted to the scheduler
	if (threads[old].status == READY) {
		SCHEDULER->admit(&threads[old]);
	}
	// deallocate the stack
	munmap(threads[old].stack, STACK_SIZE);

}

extern tid_t lwp_gettid() {
	return current;
}

extern void lwp_yield() {
	lwp_schedule(); // TODO: is this right?
}

extern void lwp_start() {
	if (SCHEDULER == NULL) {
		// print a error message
		fprintf(stderr, "No scheduler set, set it before calling me!\n");
		return;
	}
	// set up a thread and admit it to start the scheduler
	struct threadinfo_st *thread = &threads[0];
	current = thread->tid;
	SCHEDULER->admit(thread);
	lwp_schedule(); // let it rip!
}

extern void lwp_set_scheduler(scheduler fun) {
	SCHEDULER = fun;
}

extern scheduler lwp_get_scheduler() {
	return SCHEDULER;
}

extern thread tid2thread(tid_t tid) {
	if (tid < 0 || tid >= MAX_LWPS) {
		return NULL;
	}
	return &threads[tid];
}

extern tid_t lwp_wait(int *status) {
	int thread;
	tid_t tid = NO_THREAD;
	for(thread = 1; thread < MAX_LWPS; thread++) {
		if (threads[thread].status == DONE) {
			tid = thread;
			*status = threads[thread].state.rdi;
			if (status){
				*status = (int)(unsigned long)threads[thread].exited;
			}
			munmap(threads[thread].stack, STACK_SIZE);
			break;
		}
	}
}

static void lwp_schedule() {
	tid_t old = current;
	current = SCHEDULER->next()->tid;
	swap_rfiles(&threads[old].state, &threads[current].state);
	// check if old thread needs to be admitted to the scheduler
	if (threads[old].status == READY) {
		SCHEDULER->admit(&threads[old]);
	}
}