#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char* argv[])
{
	int i;

	for(i=1; i<argc; i++)
		if(i == 1) printf("%s", argv[i]);
		else printf(" %s", argv[i]);
	printf("\n");
	exit(0);
}
