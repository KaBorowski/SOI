#include "/usr/include/lib.h"
#include "/usr/include/minix/type.h"
#include <stdio.h>
#include <stdlib.h>

int getprocnr(int pid){
	message msg;
	msg.m1_i1 = pid;
	return _syscall(MM,GETPROCNR,&msg);
}

int main(int argc, char *argv[]){
	int j, index, arg;
	if (argc <= 1){
		printf("Warning: No argument given.");
		return 0;
	}
	arg = atoi(argv[1]);
	for(j=0; j<=10; j++){
		index = getprocnr(arg+j);
		if(index>0)
			printf("PID: %d - Index in mproc table: %d\n", arg+j, index);
		else
			printf("PID: %d - Process does not exist. \n", arg+j);
	}
	return 0;
}
