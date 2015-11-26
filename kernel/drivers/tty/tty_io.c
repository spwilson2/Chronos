#include <stdlib.h>
#include <string.h>

#include "kern/types.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "panic.h"
#include "tty.h"
#include "proc.h"

extern slock_t ptable_lock;
extern struct proc* rproc;
extern struct proc ptable[];

// #define DEBUG
#define KEY_DEBUG
#define QUEUE_DEBUG

static int tty_io_init(struct IODriver* driver);
static int tty_io_read(void* dst, uint start_read, size_t sz, void* context);
static int tty_io_write(void* src, uint start_write, size_t sz, void* context);
static int tty_io_ioctl(unsigned long request, void* arg, tty_t context);

int tty_io_setup(struct IODriver* driver, uint tty_num)
{
	/* Get the tty */
	tty_t t = tty_find(tty_num);
	driver->context = t;
	driver->init = tty_io_init;
	driver->read = tty_io_read;
	driver->write = tty_io_write;
	driver->ioctl = (int (*)(unsigned long, void*, void*))tty_io_ioctl;
	return 0;
}

static int tty_io_init(struct IODriver* driver)
{
	return 0;
}

static int tty_io_read(void* dst, uint start_read, size_t sz, void* context)
{
	sz = tty_gets(dst, sz, context);
	return sz;
}

static int tty_io_write(void* src, uint start_write, size_t sz, void* context)
{
	tty_t t = context;
	uchar* src_c = src;
	int x;
	for(x = 0;x < sz;x++, src_c++)
		tty_putc(t, *src_c);
	return sz;
}

static int tty_io_ioctl(unsigned long request, void* arg, tty_t t)
{
#ifdef DEBUG
	cprintf("tty: ioctl called: %d 0x%x\n", (int)request, (int)request);
#endif

	uint i = (uint)arg;
	int* ip = (int*)arg;
	struct winsize* windowp = arg;
	struct termios* termiosp = arg;
	struct termio*  termiop = arg;
	pid_t* pgidp = arg;
	pid_t* sidp = arg;

	switch(request)
	{
		case TCGETS:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
				return -1;
			memmove(arg, &t->term, sizeof(struct termios));
			break;
		case TCSETSF:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			/* Clear the input buffer */
			tty_clear_input(t);
			break;
		case TCSETSW:
		case TCSETS:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			memmove(&t->term, arg, sizeof(struct termios));
			break;
                case TCGETA:
                        if(ioctl_arg_ok(arg, sizeof(struct termio)))
				return -1;
			/* do settings */
			t->term.c_iflag = termiop->c_iflag;
			t->term.c_oflag = termiop->c_oflag;
			t->term.c_cflag = termiop->c_cflag;
			t->term.c_lflag = termiop->c_lflag;
			t->term.c_line = termiop->c_line;
			memmove(&t->term.c_cc, termiop->c_cc, NCCS);

			break;
		case TCSETAF:
			if(ioctl_arg_ok(arg, sizeof(struct termio)))
				return -1;
			tty_clear_input(t);
			/* do settings */
			t->term.c_iflag = termiop->c_iflag;
			t->term.c_oflag = termiop->c_oflag;
			t->term.c_cflag = termiop->c_cflag;
			t->term.c_lflag = termiop->c_lflag;
			t->term.c_line = termiop->c_line;
			memmove(&t->term.c_cc, termiop->c_cc, NCCS);
			break;
		case TCSETAW:
		case TCSETA:
			if(ioctl_arg_ok(arg, sizeof(struct termio)))
				return -1;
			/* do settings */
			t->term.c_iflag = termiop->c_iflag;
			t->term.c_oflag = termiop->c_oflag;
			t->term.c_cflag = termiop->c_cflag;
			t->term.c_lflag = termiop->c_lflag;
			t->term.c_line = termiop->c_line;
			memmove(&t->term.c_cc, termiop->c_cc, NCCS);
			break;
		case TIOCGLCKTRMIOS:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			if(t->termios_locked)
				memset(termiosp, 0xFF,sizeof(struct termios));
			else memset(termiosp, 0x00, sizeof(struct termios));
			break;
		case TIOCSLCKTRMIOS:
			if(ioctl_arg_ok(arg, sizeof(struct termios)))
                                return -1;
			if(termiosp->c_iflag) /* new value is locked */
				t->termios_locked = 1;
			else t->termios_locked = 0; /* new value is unlocked*/	
			break;
		case TIOCGWINSZ:
			if(ioctl_arg_ok(arg, sizeof(struct winsize)))
				return -1;
			memmove(windowp, &t->window, sizeof(struct winsize));
			break;
		case TIOCSWINSZ: /* Not allowed right now. */
			cprintf("Warning: %d tried to change the"
					" window size.\n", rproc->name);
			return -1;
			break;

			// case TIOCINQ:
		case FIONREAD:
			slock_acquire(&t->key_lock);
			int val = tty_keyboard_count(rproc->t);
			/* release lock */
			slock_release(&t->key_lock);
			return val;

		case TIOCOUTQ:
			/* No output buffer right now. */
			return 0;
		case TCFLSH:
			if(i == TCIFLUSH) tty_clear_input(t);
			else if(i == TCOFLUSH);
			else if(i == TCIOFLUSH) tty_clear_input(t);
			break;

		case TIOCSCTTY:
			/* TODO:check to make sure rproc is a session leader*/
			/* TODO:make sure rproc doesn't already have a tty*/
			if(proc_tty_connected(t))
				proc_disconnect(rproc);

			proc_set_ctty(rproc, t);
			break;

		case TIOCNOTTY:
			/* TODO: send SIGHUP to all process on tty t.*/
			if(rproc->t == t)
				proc_disconnect(rproc);
			break;

		case TIOCGPGRP:
			if(ioctl_arg_ok(arg, sizeof(pid_t)))
				return -1;
			*pgidp = t->cpgid;
			break;
		case TIOCSPGRP:
			if(ioctl_arg_ok(arg, sizeof(pid_t)))
                                return -1;
                        t->cpgid = *pgidp;
			break;
		case TIOCGSID:
			/* TODO: make sure this is correct */
			if(ioctl_arg_ok(arg, sizeof(pid_t)))
				return -1;
			*sidp = rproc->sid;
			break;
		case TIOCEXCL:
			/* TODO: enforce exlusive mode */
			t->exclusive = 1;
			break;
		case TIOCGEXCL:
			if(ioctl_arg_ok(arg, sizeof(int)))
                                return -1;
			*ip = t->exclusive;
			break;
		case TIOCNXCL:
			t->exclusive = 0;
			break;
		case TIOCGETD:
			if(ioctl_arg_ok(arg, sizeof(int)))
                                return -1;
			*ip = t->term.c_line;
			break;
		case TIOCSETD:
			if(ioctl_arg_ok(arg, sizeof(int)))
                                return -1;
			t->term.c_line = *ip;
			break;
	
		case TCXONC: /* Suspend tty */
		case TCSBRK:
		case TCSBRKP:
		case TIOCSBRK:
		case TIOCCBRK:
			cprintf("Warning: %s used ioctl feature"
					" that has no effect: %d\n", 
					rproc->name, request);
			break;
		default:
			cprintf("Warning: ioctl request not implemented:"
					" 0x%x\n", request);
			return -1;
	}

#ifdef DEBUG
	cprintf("tty: ioctl successful!\n");
#endif
	return 0;	
}

static struct proc* tty_dequeue(tty_t t)
{
	struct proc* p = NULL;
	/* Grab the IO queue lock */
	slock_acquire(&t->io_queue_lock);
	if(t->io_queue)
	{
		p = t->io_queue;
#ifdef QUEUE_DEBUG
		cprintf("tty_queue: %s dequeued.\n", p->name);
#endif
		if(p)
		{
			t->io_queue = p->io_next;
			p->io_next = NULL;
		} else {
			t->io_queue = NULL;
		}
	} else {
#ifdef QUEUE_DEBUG
		cprintf("tty_queue: NULL DEQUEUE!!.\n", p->name);
#endif
	}
	slock_release(&t->io_queue_lock);

	return p;
}

static void tty_enqueue(struct proc* p, tty_t t)
{
	/* Grab the IO queue lock */
	slock_acquire(&t->io_queue_lock);

#ifdef QUEUE_DEBUG
	cprintf("tty_queue: %s enqueued.\n", p->name);
#endif

	if(!t->io_queue)
	{
		t->io_queue = p;
	} else {
		struct proc* next = t->io_queue;
		while(next->io_next)
			next = next->io_next;
		next->io_next = p;
	}
	slock_release(&t->io_queue_lock);
}

int tty_gets(char* dst, int sz, tty_t t)
{
	/* Check for bad request */
	if(!sz || !dst) return 0;

	/* Acquire the ptable lock */
	slock_acquire(&ptable_lock);

	rproc->io_dst = (void*)dst;
	memset(dst, 0, sz);

io_sleep:
	/* Put our process in the wait queue */
	tty_enqueue(rproc, t);

	/* Go to sleep */
	rproc->state = PROC_BLOCKED;
	rproc->block_type = PROC_BLOCKED_IO;
	rproc->io_recieved = 0;
	rproc->io_request = sz;

#ifdef KEY_DEBUG
	cprintf("tty: %s is now waiting for io.\n", 
		rproc->name);
#endif

	yield_withlock();

#ifdef KEY_DEBUG
        cprintf("tty: %s is now running again!\n",
                rproc->name);
#endif

	/* When we wake up here, we should have something in our buffer. */
	slock_acquire(&ptable_lock);

	/* Did we get anything? */
	if(rproc->io_recieved == 0)
	{
		cprintf("tty: extraneous proc wakeup.\n");
		goto io_sleep;
	}

	slock_release(&ptable_lock);

	return rproc->io_recieved;
}

/* ptable and key lock must be held here. */
void tty_signal_io_ready(tty_t t)
{
	char canon = t->term.c_lflag & ICANON;

	/* Keyboard signaled. */
	struct proc* p = tty_dequeue(t);
	if(!p)
	{
#ifdef KEY_DEBUG
		cprintf("tty: signaled, nobody is waiting!\n");
		return;
#endif
	}

	/* Read into the destination buffer */
	int read_bytes = p->io_recieved;
	int wake = 0; /* Wether or not to wake the process */

#ifdef KEY_DEBUG
	cprintf("tty: %s starting read with %d bytes.\n",
			p->name, read_bytes);
#endif


	for(read_bytes = 0;read_bytes < p->io_request;
			read_bytes++)
	{
		/* Read a character */
		char c = tty_keyboard_read(t);
		if(!c) break;
		/* Place character into dst */
#ifdef KEY_DEBUG
		cprintf("tty: %c given to %s\n",
				c, p->name);
#endif
		p->io_dst[read_bytes] = c;
		if(canon && (c == '\n' || c == 13))
		{
			read_bytes++;
			break;
		}
	}

	/* If we read anything, wakeup the process */
	if(read_bytes > 0) wake = 1;

#ifdef KEY_DEBUG
        cprintf("tty: %s ended read with %d bytes.\n",
                        p->name, read_bytes);
#endif

	/* update recieved */
	p->io_recieved = read_bytes;

	if(wake)
	{
#ifdef KEY_DEBUG
		cprintf("tty: %s woke up!\n", p->name);
#endif
		p->state = PROC_RUNNABLE;
		p->block_type = PROC_BLOCKED_NONE;
	}
}
