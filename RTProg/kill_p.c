#include <stdio.h>
#include <signal.h>

/*
Kill zombie processes that are not killed by the init process
Helper application
*/
int main(int argc, char *argv[])
{
	int tmp, tmp2, count;
	int i;
	
	tmp = atoi(argv[1]);
	tmp2 = atoi(argv[2]);
	count = tmp2 - tmp;
	
	for(i=0; i<(count+1); i++){
		kill(tmp, SIGKILL);
		tmp = tmp + 1;
		}
	return 1;
}
