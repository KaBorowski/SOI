#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "monitor.h"

#define N 9
#define TAKE 2

class BufferMonitor:Monitor{
private:
  char buffer[N];
  Condition full;
  Condition empty;
  int bufferSize;
public:
  BufferMonitor();
  void put(char value);
  char get(char value);
};

BufferMonitor::BufferMonitor(){
  bufferSize = 0;
}

void BufferMonitor::put(char value){
  enter();
  if (bufferSize == N){
    std::cout << "Producent " << value << " czeka na zwolnienie miejsca w buforze\n";
    wait(full);
  }
  buffer[bufferSize] = value;
  std::cout << "Producent " << value << " dodal element do bufora\n";
  ++bufferSize;
  if (bufferSize == 1) signal(empty);
  leave();
}

char BufferMonitor::get(char value){
  char ret;
  enter();
  if (bufferSize == 0){
    std::cout << "Konsument " << value << " czeka na uzupelnienie bufora\n";
    wait(empty);
  }
  ret = buffer[0];
  std::cout << "Konsument " << value << " odczytal element  " << ret << "\n";
  for (int i = 0; i < bufferSize; i++){
    buffer[i] = buffer[i+1];
  }
  --bufferSize;
  if (bufferSize == N-1) signal(full);
  leave();
  return ret;
}

BufferMonitor buffMon;

class ProducerMonitor:Monitor{
public:
  void produce(char value);
};

void ProducerMonitor::produce(char value){
  enter();
  buffMon.put(value);
  leave();
}

class ConsumerMonitor:Monitor{
public:
  void consume(char value);
};

void ConsumerMonitor::consume(char value){
  enter();
  for (int i = 0; i < TAKE; i++)
    buffMon.get(value);
  leave();
}

ProducerMonitor prodMon;
ConsumerMonitor consMon;

void * producer (void * args){
  char value = *((char *)args);
  while (true) {
    prodMon.produce(value);
    usleep(100000);
  }
}

void * consumer (void * args){
  char value = *((char *)args);
  while (true) {
    consMon.consume(value);
    usleep(1000000);
  }
}

int main(int argc, char const *argv[]) {
 pthread_t pA, pB, pC, pD;
 char a = 'a';
 char b = 'b';
 char c = 'c';
 char d = 'd';

  pthread_create(&pA, NULL, producer, &a);
  pthread_create(&pB, NULL, producer, &b);
  pthread_create(&pC, NULL, consumer, &c);
  pthread_create(&pD, NULL, consumer, &d);

  pthread_join(pA, NULL);
  pthread_join(pB, NULL);
  pthread_join(pC, NULL);
  pthread_join(pD, NULL);

  return 0;
}
