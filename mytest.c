/* Single Author info:
 * 	(All of us contributed equal share)
 *	rrshah3  Rhythm Shah
 *	spshriva Shalki Shrivastava
 *	smsejwan Shalini Sejwani
 * Group info:
 *      rrshah3  Rhythm Shah
 *		spshriva Shalki Shrivastava
 *		smsejwan Shalini Sejwani
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/syscall.h>

#include "mythread.h"
#include "mymutex.h"
#include <limits.h>

#define NTHREADS	10
#define SETCONCURRENCY	5

#define MYLIMIT	50000

struct futex printf_fut;
mythread_mutex_t mymutex;
int gcount = 0;

void *thread_func(void *arg)
{
	int count = *(int *)arg;
	mythread_t self = mythread_self();
	int i;

	while(1) {
		mythread_mutex_lock(&mymutex);
		if (gcount < MYLIMIT) {
			for(i = 0; i < INT_MAX/1000; i++);
			gcount += 50;
			mythread_mutex_unlock(&mymutex);
		} else {
			mythread_mutex_unlock(&mymutex);
			break;
		}
	}
	if (count % 2)
		mythread_exit(0);

	return NULL;
}

int main()
{
	mythread_t threads[NTHREADS];
	int count[NTHREADS];
	int i;
	char *status;
	
	mythread_mutex_init(&mymutex, NULL);
	mythread_setconcurrency(SETCONCURRENCY);

	printf("Wait for some time for the final output. It will appear.)\n");

	for (i = 0; i < NTHREADS; i++) {
		count[i] = i;
		mythread_create(&threads[i], NULL, thread_func, &count[i]);
	}

	mythread_t self = mythread_self();

	mythread_enter_kernel();
	printf("I am main function: %ld\n", (long int)self->tid);
	mythread_leave_kernel();

	for (i = 0; i < NTHREADS; i++) {
		mythread_enter_kernel();
		printf("Main: Will now wait for thread %ld. Yielding..\n", (long int)threads[i]->tid);
		mythread_leave_kernel();
		mythread_join(threads[i], (void **)&status);
	}
	printf("All threads completed execution.\nFinal count value:%d. \nWill now exit.\n", gcount);
	mythread_exit(NULL);

	return 0;
}
