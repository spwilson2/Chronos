#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "file.h"
#include "stdlock.h"
#include "chronos.h"

int main(int argc, char* argv[])
{
	char buffer[256];
	if(cwd(buffer, 256) == -1)
	{
		exit();
	}
	printf("%s \n", buffer);
	exit();
}
