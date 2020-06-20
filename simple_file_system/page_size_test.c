#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

void main (){
	long res = sysconf(_SC_PAGE_SIZE);
	printf("Page size = %ld\n", res);
	
}
