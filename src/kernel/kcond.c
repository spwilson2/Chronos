#include <chronos/stdlock.h>
#include <chronos/file.h>
#include <chronos/syscall.h>
#include <chronos/pipe.h>
#include <chronos/devman.h>
#include <chronos/tty.h>
#include <chronos/fsman.h>
#include <chronos/proc.h>

void cond_init(cond_t* c)
{
	c->next_signal = 0;
	c->current_signal = 0;
}

void cond_signal(cond_t* c)
{
	int* esp_saved = rproc->sys_esp;
	rproc->sys_esp = (int*)&c;
	sys_signal_cv();
	rproc->sys_esp = esp_saved;
}

void cond_wait_spin(cond_t* c, slock_t* lock)
{
	int* esp_saved = rproc->sys_esp;
        rproc->sys_esp = (int*)&c;
	sys_wait_s();
	rproc->sys_esp = esp_saved;
}

void cond_wait_ticket(cond_t* c, tlock_t* lock)
{
	int* esp_saved = rproc->sys_esp;
        rproc->sys_esp = (int*)&c;
	sys_wait_t();
	rproc->sys_esp = esp_saved;
}
