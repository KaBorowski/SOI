#include <stdio.h>
#include <stdlib.h>

struct fat{
  int * fatTable;
};
struct dentry{
  char name [32];
  unsigned int size;
  time_t create, modify;
  unsigned int firstBlock;
  unsigned int valid;
};

int main(int argc, char const *argv[]) {
  struct fat * f;
  struct dentry * df;
  int * i;
  printf("%ld\n", sizeof(struct fat));
  printf("%ld\n", sizeof(f));
  f = malloc(sizeof(struct fat));
  i = malloc(sizeof(int));
  f->fatTable = i;
  printf("%ld\n", sizeof(f));
  printf("%ld\n", sizeof(struct dentry));
  i[0] = 1;
  printf("%d\n", i[0]);
  i[10] = 2;
  printf("%d\n", i[10]);
  return 0;
}
