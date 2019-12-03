#include <stdio.h>
#include <unistd.h>

int main()
{
	printf("%ld\n", (0x7fff76a0d000%sysconf(_SC_PAGESIZE)));
	return 0;
}
