#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int tmp, tmp2, count;
	int i;
	char fifo[10];
	
	tmp = atoi(argv[1]);
	tmp2 = atoi(argv[2]);
	count = tmp2 - tmp;
	
	for(i=0; i<(count+1); i++){
		sprintf(fifo, "%d", tmp);
		unlink(fifo);
		tmp = tmp + 1;
		}
	return 1;
}


