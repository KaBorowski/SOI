#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char * getDate(time_t time){
  struct tm * date;
  date = localtime(&time);
  return asctime(date);
}

int main(int argc, char const *argv[]) {
  time_t tm, tm2;
  tm = time(NULL);
  tm2 = time(NULL)-1000;

  printf("%s %s\n", getDate(tm),getDate(tm2));


  return 0;
}
