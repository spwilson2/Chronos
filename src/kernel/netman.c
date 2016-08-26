#include <stdlib.h>
#include <string.h>

#include <chronos/kstdlib.h>
#include <chronos/netman.h>

char hostname[HOSTNAME_LEN];

int net_init(void)
{
	strncpy(hostname, "chronos-1.0", HOSTNAME_LEN);
	return 0;
}
