#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "monitor.h"

#define N 9

char buffer[N];
int input = 0;
int output = 0;
int counter = 0;

Semaphore empty = Semaphore(N);
Semaphore reader = Semaphore(1);
Semaphore writer = Semaphore(1);
Semaphore full = Semaphore(0);
Semaphore mutex = Semaphore(1);

void producer(char arg){
  while(true){
    //produce
    char value = arg;
    bool canRead = false;
    printf("%c czeka na SK\n", value);
    empty.p();
    //mutex.p();
    writer.p();
    reader.p();
    if(input!=output && input!=(output+1)%N && input!=(output+2)%N){
      canRead = true;
      reader.v();
    }
    printf("%c wchodzi do SK\n", value);
    buffer[input] = value;
    input = (input+1)%N;
    mutex.p();
    counter++;
    mutex.v();
    printf("%c opuszcza SK\n", value);
    if(counter>2)
      full.v();
    if (!canRead)
      reader.v();
    //mutex.v();
    writer.v();
    usleep(1000000);
  }
}
void consumer(){
  while(true){
    char value[3];
    printf("Konsument czeka na SK\n");
    int i;
    full.p();
    //mutex.p();
    reader.p();
    printf("Konsument wchodzi do SK\n");
    mutex.p();
    counter=counter-3;
    mutex.v();
    for(i=0; i<3; i++){
      value[i] = buffer[output];
      output = (output+1)%N;
    }
    printf("Konsument opuszcza SK z %c, %c, %c\n", value[0],value[1],value[2]);
    //mutex.v();
    for(i=0; i<3; i++){
      empty.v();
    }
    reader.v();
    usleep(900000);
  }
}
void newConsumer(){
  int created = fork();
  if(created == 0){
    consumer();
    exit(0);
  }
}
void newProducent(char write){
  int created = fork();
  if(created == 0){
    producer(write);
  }
}
int main(int argc, char const *argv[]) {
  newConsumer();
  newProducent('a');
  return 0;
}
