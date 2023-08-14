#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include "tpool.h"

tpool_t*
initPool(int numThreads)
{
	tpool_t *newPool;

	newPool = malloc(sizeof(tpool_t));
	newPool->threads = malloc(sizeof(thrd_t) * numThreads);
	newPool->thread_args = malloc(sizeof(struct thread_info) * numThreads);
	newPool->numThreads = numThreads;

	for (int i = 0; i < numThreads; ++i) {
		newPool->thread_args[i].args = NULL;
		newPool->thread_args[i].func = NULL;
		atomic_store(&newPool->thread_args[i].active, 0);
		atomic_store(&newPool->thread_args[i].kill, 0);
		thrd_create(&newPool->threads[i], defaultThread, &newPool->thread_args[i]);
	}

	return newPool;
}

int
defaultThread(void *arg)
{
	struct thread_info *info = arg;
	for (;;) {
		if (atomic_load(&info->active)) {
			info->func(info->args);
			atomic_store(&info->active, 0);
		}
		else if (atomic_load(&info->kill))
			goto EXIT;
	}
EXIT:
	return 0;
}

void
addWork(tpool_t *pool, thread_func_t func, void *args)
{
	int n = pool->numThreads;
	uint8_t found = 0;
	while (!found) {
		for (int i = 0; i < n; ++i) {
			if (!atomic_load(&pool->thread_args[i].active)) {
				found = 1;
				pool->thread_args[i].func = func;
				pool->thread_args[i].args = args;
				atomic_store(&pool->thread_args[i].active, 1);
				i = n;
			}
		}
	}
}

void
killPool(tpool_t *pool)
{
	assert(pool);
	int numThreads = pool->numThreads;
	for (int i = 0; i < numThreads; ++i)
		atomic_store(&pool->thread_args[i].kill, 1);
	for (int i = 0; i < numThreads; ++i)
		thrd_join(pool->threads[i], NULL);
	free(pool->threads);
	free(pool->thread_args);
	free(pool);
}
