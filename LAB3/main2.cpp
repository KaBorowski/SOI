#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define N 9

// char buffer[N];
// int input = 0;
// int output = 0;
// int counter = 0;

struct Semaphores{
  sem_t empty;
  sem_t full;
  sem_t reader;
  sem_t writer;
  sem_t mutex;
};
struct Semaphores * getSemaphores()
{
    /*funkcja przydziela strukturze Semaphore pamiec wspoldzielona / zwraca polozenie w pamieci wspoldzielonej */
    static int shmid = 0;

    if(shmid == 0)
            shmid = shmget(IPC_PRIVATE,sizeof(struct Semaphores) + sizeof(sem_t),SHM_W|SHM_R);

    if(shmid < 0)
    {
        printf("shmget error\n");
        abort();
    }

    void * Data = shmat(shmid, NULL, 0);
    struct Semaphores * semaphores = (struct Semaphores *) Data;

    return semaphores;
}

struct Buffer{
  char buff[N];
  unsigned int bufferSize;
};
struct Buffer * getBuffers()
{/*funkcja przydziela buforom miejsce w pamieci wspoldzielonej / zwraca polozenie w pamieci wspoldzielonej */
    static int shmid = 0;
    unsigned int i;

    if(shmid == 0)
    {
        shmid = shmget(IPC_PRIVATE, sizeof(struct Buffer) + N * sizeof(unsigned int), SHM_W|SHM_R);//IPC_PRIVATE zapewnia niepowtarzalny identyfikator
    }

    if(shmid < 0)
    {
        printf("shmget error\n");
        abort();
    }

    void * Data = shmat(shmid,NULL,0);

    struct Buffer * buffers = (struct Buffer *)Data;

    return buffers;
}

void initSem(){
  struct Buffer * buffer = getBuffers();
  struct Semaphores * semaphores = getSemaphores();

  buffer->bufferSize = 0;

  sem_init(&semaphores->empty, 1, N);
  sem_init(&semaphores->full, 1, 0);
  sem_init(&semaphores->reader, 1, 1);
  sem_init(&semaphores->writer, 1, 1);
  sem_init(&semaphores->mutex, 1, 1);

}

void superProducer(char arg){
  struct Buffer * buffer = getBuffers();
  struct Semaphores * semaphores = getSemaphores();
  char ret;

  while(1){
    printf("SuperProducent %c czeka na reader\n", arg);
    sem_wait(&semaphores->reader);
    printf("SuperProducent %c czeka na full\n", arg);
    sem_wait(&semaphores->full);
    printf("SuperProducent %c czeka na writer\n", arg);
    sem_wait(&semaphores->writer);
    sem_wait(&semaphores->mutex);
    ret = buffer->buff[0];
    for(int j=0; j<buffer->bufferSize; j++){
      buffer->buff[j] = buffer->buff[j+1];
    }
    --buffer->bufferSize;
    buffer->buff[buffer->bufferSize] = arg;
    ++buffer->bufferSize;
    printf("SuperProducent %c opuszcza SK\n", arg);
    sem_post(&semaphores->mutex);
    sem_post(&semaphores->full);
    sem_post(&semaphores->writer);
    sem_post(&semaphores->reader);
    usleep(1100000);
  }
}

void producer(char arg){
  struct Buffer * buffer = getBuffers();
  struct Semaphores * semaphores = getSemaphores();

  while(1){
    printf("Producent %c czeka na writer\n", arg);
    sem_wait(&semaphores->writer);
    printf("Producent %c czeka na full\n", arg);
    sem_wait(&semaphores->empty);
    printf("Producent %c czeka na mutex\n", arg);
    sem_wait(&semaphores->mutex);
    printf("Producent %c produkuje\n", arg);
    buffer->buff[buffer->bufferSize] = arg;
    ++buffer->bufferSize;
    printf("Producent %c opuszcza SK\n", arg);
    sem_post(&semaphores->mutex);
    sem_post(&semaphores->full);
    sem_post(&semaphores->writer);
    usleep(900000);
  }
}

void consumer(char id, unsigned int number){
  struct Buffer * buffer = getBuffers();
  struct Semaphores * semaphores = getSemaphores();
  char ret;

  while(1){
    printf("Konsument %c czeka na reader\n", id);
    sem_wait(&semaphores->reader);

    for(int i=0; i<number; i++){
      printf("Konsument %c czeka na full\n", id);
      sem_wait(&semaphores->full);
      printf("Konsument %c czeka na mutex\n", id);
      sem_wait(&semaphores->mutex);

      printf("Konsument %c pobiera %d/%d\n", id, i+1, number);
      ret = buffer->buff[0];
      for(int j=0; j<buffer->bufferSize; j++){
        buffer->buff[j] = buffer->buff[j+1];
      }
      --buffer->bufferSize;
      sem_post(&semaphores->mutex);
      sem_post(&semaphores->empty);
    }
    printf("Konsument %c wychodzi z SK\n", id);
    sem_post(&semaphores->reader);
    usleep(1000000);
  }
}

void newConsumer(char id, unsigned int number){
  int created = fork();
  if(created == 0){
    consumer(id, number);
    exit(0);
  }
}
void newProducent(char write){
  int created = fork();
  if(created == 0){
    producer(write);
  }
}

void newSuperProducent(char write){
  int created = fork();
  if(created == 0){
    superProducer(write);
  }
}

int main(int argc, char const *argv[]) {
  initSem();
  newConsumer('a', 2);
  newProducent('b');
  newConsumer('c', 3);
  newProducent('d');
  newSuperProducent('s');

  while(1){}

  return 0;
}
