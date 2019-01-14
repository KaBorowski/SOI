#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <lib.h>
#include <minix/type.h>

int main(int argc, char* argv[]) {

  message input;
  pid_t pid = getpid();
  int i, j;
  int value = atoi(argv[2]);
  
  input.m1_i1 = pid;
  input.m1_i2 = atoi(argv[1]);
  input.m1_i3 = atoi(argv[2]);
  _syscall(MM, SETPRI, &input);

  sleep(3);
  /*aby lepiej ogladac dzialanie programu,
   * mozna zwiekszyc liczbe wykonan zewnetrznej petli */
  for(j=0; j<1; ++j) 
    for(i=1; i<9999999; ++i){

    }
  return 0;
}
