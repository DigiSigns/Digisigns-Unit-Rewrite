#ifndef TPOOL_H
#define TPOOL_H

#include <stdatomic.h>
#include <stdint.h>
#include <threads.h>

typedef void (*thread_func_t)(void*);

struct thread_info {
	void *args;
	thread_func_t func;
	_Atomic int8_t active;
	_Atomic int8_t kill;
};

typedef struct tpool {
	thrd_t *threads;
	struct thread_info *thread_args;
	int numThreads;
} tpool_t;

tpool_t*
initPool(int);

int
defaultThread(void *);

void
addWork(tpool_t*, thread_func_t, void*);

void
killPool(tpool_t*);

#endif //TPOOL_H
