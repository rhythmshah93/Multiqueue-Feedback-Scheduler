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
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "mythread.h"
#include "myqueue.h"
#include <math.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#define DEFAULT_ATTR 0
#define SLEEPING	0x2
#define IN_SIGALRM	0x4
#define	IN_SIGUSR1	0x8

static int mythread_scheduler();

static void signal_handler(int sig)
{
	mythread_t self = mythread_self();
	mythread_queue_t ptr;
	ptr = *mythread_runq();
	if (ptr != NULL) {
		if (sig == SIGALRM)
			self->state |= IN_SIGALRM;
		else
			self->state |= IN_SIGUSR1;
		self->reschedule = 1;

		if(mythread_tryenter_kernel() == TRUE) {
			if (self->reschedule == 0)
				mythread_leave_kernel_nonpreemptive();
			else 
				mythread_leave_kernel();
		}
	}	
}

void mythread_leave_kernel()
{
	mythread_t self = mythread_self();

retry:
	if (self->reschedule == 1) {
		self->attr.attr++;
		//printf("new attr is %d \n",self->attr.attr);
		self->reschedule = 0;
		if (self->state & SLEEPING || mythread_inq(mythread_runq(), self) == FALSE) {
			mythread_leave_kernel_nonpreemptive();
		} else if (mythread_scheduler() != 0) {
			if(self->attr.attr==pow(2,self->preemptions))
			{
				if(!self->preemptions==9)self->preemptions = self->preemptions + 1;
				self->attr.attr=0;
			}			
			mythread_leave_kernel_nonpreemptive();
		} else if(self->attr.attr==pow(2,self->preemptions)){

			self->preemptions = self->preemptions + 1;
			self->attr.attr=0;
			void* r = mythread_readyq();
			mythread_t rthread = (mythread_t)r;
			if(rthread->attr.attr<=self->attr.attr)
			{
				mythread_block(mythread_readyq(), SLEEPING);
				self->state &= (~SLEEPING);	
			}
			else mythread_leave_kernel_nonpreemptive();
		}
		else mythread_leave_kernel_nonpreemptive();
	} else {
		mythread_leave_kernel_nonpreemptive();
	}

	if (self->reschedule == 1) {
		 mythread_enter_kernel();
			goto retry;
	}
}

static int mythread_scheduler()
{
	mythread_queue_t ptr, head;
	mythread_t self = mythread_self();
	if (self->state & IN_SIGALRM) {
		head = *mythread_runq();
		ptr = head;
		do {
			if (self != ptr->item) {
				syscall(SYS_tkill, ((mythread_t)ptr->item)->tid, SIGUSR1);
			}
			ptr = ptr->next;
		} while(ptr != head); 
	}
	self->state &= ~IN_SIGALRM;
	self->state &= ~IN_SIGUSR1;

	if (*mythread_readyq() != NULL)
		return 0;
	else 
		return -1;

}

struct sigaction sig_act;
struct sigaction old_sig_act;

sigset_t newmask;
sigset_t oldmask;

static int timer_initialised = 0;

void mythread_init_sched()
{
	struct itimerval timer;
	struct timeval timerval;

	memset(&sig_act, '\0', sizeof(sig_act));
	sig_act.sa_handler = signal_handler;

	if (sigaction(SIGALRM, &sig_act, &old_sig_act) == -1) {
		printf("Error in registering the Signal Handler for SIGALRM!\n");
		printf("Exiting....");
		exit(-1);
	}

	if (sigaction(SIGUSR1, &sig_act, &old_sig_act) == -1) {
		printf("Error in registering the Signal Handler for SIGUSR1!\n");
		printf("Exiting....");
		exit(-1);
	}

	sigemptyset(&newmask);
	sigaddset(&newmask, SIGALRM);
	sigaddset(&newmask, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &newmask, &oldmask);

	if (timer_initialised == 0) {
		timer_initialised = 1;
		timerval.tv_sec = 0;
		timerval.tv_usec = 1000;
		timer.it_interval = timerval;
		timer.it_value = timerval;
		setitimer(ITIMER_REAL, &timer, NULL);
	}
}

void mythread_exit_sched()
{
	if (sigaction(SIGUSR1, &old_sig_act, &sig_act) == -1) {
		printf("Error in removing the signal handler for SIGUSR1!\n");
		printf("Exiting....\n");
		exit(-1);
	}

	if (sigaction(SIGALRM, &old_sig_act, &sig_act) == -1) {
		printf("Error in removing the Signal Handler for SIGALRM!\n");
		printf("Exiting....\n");
		exit(-1);
	}

	sigprocmask(SIG_SETMASK, &oldmask, &newmask);
}

