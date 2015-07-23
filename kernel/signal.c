#include "types.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdlock.h"
#include "trap.h"
#include "signal.h"
#include "file.h"
#include "fsman.h"
#include "devman.h"
#include "pipe.h"
#include "tty.h"
#include "proc.h"
#include "panic.h"
#include "vm.h"
#include "cpu.h"
#include "chronos.h"

extern struct proc* rproc;
extern struct proc ptable;
extern slock_t ptable_lock;

/* Signal default action table */
static int default_actions[] =
{
	SIGDEFAULT_KPANIC, 	/* 0 = invalid signal */
	SIGDEFAULT_TERM,	/* 1 = SIGHUP */
	SIGDEFAULT_TERM,	/* 2 = SIGINT */
	SIGDEFAULT_CORE,	/* 3 = SIGQUIT */
	SIGDEFAULT_CORE,	/* 4 = SIGILL */
	SIGDEFAULT_CORE,	/* 5 = SIGTRAP */
	SIGDEFAULT_CORE,	/* 6 = SIGABRT */
	SIGDEFAULT_CORE,	/* 7 = SIGBUS */
	SIGDEFAULT_CORE,	/* 8 = SIGFPE */
	SIGDEFAULT_TERM,	/* 9 = SIGKILL */
	SIGDEFAULT_TERM,	/* 10 = SIGUSR1 */
	SIGDEFAULT_CORE,	/* 11 = SIGSEGV */
	SIGDEFAULT_TERM,	/* 12 = SIGUSR2 */
	SIGDEFAULT_TERM,	/* 13 = SIGPIPE */
	SIGDEFAULT_TERM,	/* 14 = SIGALRM */
	SIGDEFAULT_TERM,	/* 15 = SIGTERM */
	SIGDEFAULT_TERM,	/* 16 = SIGSTKFLT */
	SIGDEFAULT_IGN, 	/* 17 = SIGCHLD */
	SIGDEFAULT_CONT,	/* 18 = SIGCONT */
	SIGDEFAULT_STOP,	/* 19 = SIGSTOP */
	SIGDEFAULT_STOP,	/* 20 = SIGTSTP */
	SIGDEFAULT_STOP,	/* 21 = SIGTTIN */
	SIGDEFAULT_STOP,	/* 22 = SIGTTOU */
	SIGDEFAULT_IGN, 	/* 23 = SIGURG */
	SIGDEFAULT_CORE,	/* 24 = SIGXCPU */
	SIGDEFAULT_CORE,	/* 25 = SIGXFSZ */
	SIGDEFAULT_TERM,	/* 26 = SIGVTALRM */
	SIGDEFAULT_TERM,	/* 27 = SIGPROF */
	SIGDEFAULT_IGN, 	/* 28 = SIGWINCH */
	SIGDEFAULT_TERM,	/* 29 = SIGIO */
	SIGDEFAULT_TERM,	/* 30 = SIGPWR */
	SIGDEFAULT_CORE 	/* 31 = SIGSYS */
	
};

/* Signal table */
static struct signal_t sigtable[MAX_SIGNAL];
static slock_t sigtable_lock;

static struct signal_t* sig_alloc(void)
{
	slock_acquire(&sigtable_lock);
	
	struct signal_t* sig = NULL;
	int x;
	for(x = 0;x < MAX_SIGNAL;x++)
	{
		if(!sigtable[x].allocated)
		{
			memset(sigtable + x, 0, sizeof(struct signal_t));
			sigtable[x].allocated = 1;
			sig = sigtable + x;
			break;
		}
	}

	if(!sig)
		cprintf("kernel: warning: signal table full.\n");

	slock_release(&sigtable_lock);

	return sig;
}

static void sig_free(struct signal_t* sig)
{
	slock_acquire(&sigtable_lock);
	if(!sig) panic("kernel: tried to free NULL signal.\n");
	if(sig < sigtable || sig >= sigtable + MAX_SIGNAL)
		panic("kernel: tried to free invalid signal.\n");
	sig->allocated = 0;
	slock_release(&sigtable_lock);
}

void sig_init(void)
{
	slock_init(&sigtable_lock);
	memset(sigtable, 0, sizeof(struct signal_t) * MAX_SIGNAL);
}

static void sig_queue(struct proc* p, struct signal_t* sig)
{
	struct signal_t* ptr = p->sig_queue;
	if(!ptr)
	{
		p->sig_queue = sig;
		sig->next = NULL;
		return;
	}
}

static void sig_dequeue(struct proc* p)
{
	struct signal_t* ptr = p->sig_queue;
	if(ptr)
	{ 
		p->sig_queue = ptr->next;
		sig_free(ptr);
	}
}

int sig_proc(struct proc* p, int sig)
{
	if(sig < SIG_FIRST || sig > SIG_LAST)
		return -1;
	if(!p) return -1;

	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);

	/* Get a new signal */
	struct signal_t* signal = sig_alloc();
	if(!signal)
	{
		slock_release(&ptable_lock);
		return -1;
	}

	/* Setup signal */
	signal->type = sig;
	signal->default_action = default_actions[sig];
	signal->next = NULL;

	if(sig != SIGKILL && sig != SIGSTOP)
		signal->catchable = 1;

	/* Queue up this signal */
	sig_queue(p, signal);

	/* Change process state to signaled */
	p->state = PROC_SIGNALED;

	/* give up ptable lock */
	slock_release(&ptable_lock);

	return 0;
}

int sig_handle(void)
{
	/* Make sure there is a signal being waited on. */
	if(!rproc->sig_queue)
		return 0;

	/* Not sure if interrupts will mess this up but better be sure */
	push_cli();
	slock_acquire(&ptable_lock);

	struct signal_t* sig = rproc->sig_queue;
	void (*sig_handler)(int sig_num) = rproc->signal_handler;
	uchar caught = 0; /* Did we catch the signal? */
	uchar terminated = 0; /* Did we get terminated? */
	uchar stopped = 0; /* Did we get stopped? */
	uchar core = 0; /* Should we dump the core? */

	switch(rproc->sig_queue->default_action)
	{
		case SIGDEFAULT_KPANIC:
			panic("kernel: Invalid signal handled!\n");
			break;
		case SIGDEFAULT_TERM:
			if(sig->catchable && sig_handler)
			{
				caught = 1;
			} else {
				terminated = 1;
			}
			break;
		case SIGDEFAULT_CORE:
			if(sig->catchable && sig_handler)
			{
                                caught = 1;
			} else { 
                                core = terminated = 1;
			}
			break;
		case SIGDEFAULT_STOP:
			if(sig->catchable && sig_handler)
                        {
                                caught = 1;
                        } else {
                        	stopped = 1;
                        }
			break;
		case SIGDEFAULT_CONT:
			rproc->state = PROC_RUNNABLE;
			if(sig->catchable && sig_handler)
                        {
                                caught = 1;
                        }
			break;
		case SIGDEFAULT_IGN:
			if(sig->catchable && sig_handler)
                        {
                                caught = 1;
                        }
			break;
	}

	if(caught)
        {
		/**
		 * WARNING: SEPERATE SIGNAL STACKS NEEDED
		 */

                /* We need to save some state information */
                memmove(rproc->tf, &sig->handle_frame, 
			sizeof(struct trap_frame));
		/* set the return address to the sig handler */
		rproc->tf->eip = (uint)sig_handler;

		/* (safely) Push our magic return value */
		uint esp = (uint)rproc->tf->esp;
		esp -= 4;
		uint mag = SIG_MAGIC;
		vm_memmove((void*)esp, &mag, sizeof(int), 
			rproc->pgdir, rproc->pgdir);

		/* Update stack pointer */
		rproc->tf->esp = esp;
        }

	/* Check to see if we need to dump the core */
	if(core)
	{
		/* Dump memory for gdb analysis */
	}
	
	pop_cli();
	
	/* If we got stopped or terminated, we will enter scheduler. */
	if(stopped || terminated)
	{
		sig_dequeue(rproc);
		yield_withlock();
	}else slock_release(&ptable_lock);

	return 0;
}