#include <stdlib.h>
#include <string.h>

#include <chronos/kstdlib.h>
#include <chronos/stdlock.h>
#include <chronos/file.h>
#include <chronos/syscall.h>
#include <chronos/devman.h>
#include <chronos/fsman.h>
#include <chronos/netman.h>
#include <chronos/syscall.h>
#include <chronos.h>
#include <chronos/pipe.h>
#include <chronos/tty.h>
#include <chronos/proc.h>
#include <chronos/panic.h>

int sys_gethostname(void)
{
	char* buffer;
	size_t size;
	
	if(syscall_get_int((int*)&size, 1)) return 0;
	if(syscall_get_buffer_ptr((void**)&buffer, size, 0)) return 0;

	strncpy(buffer, hostname, size);
	
	return 0;
}
