#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define BLOCK_SIZE 2048
#define MAX_NAME_SIZE 64


struct superblock{
  unsigned int diskSize;
  char name[MAX_NAME_SIZE];
  unsigned int fatBlockSize;
  unsigned int rootdirBlockSize;
  unsigned int blockSize;
};

struct vfs{
  FILE * file;
  unsigned int blockAmount;
  struct dentry * rd;
};

struct dentry{
  char name [MAX_NAME_SIZE];
  unsigned int size;
  time_t create;
  time_t modify;
  int firstBlock;
  unsigned int valid;
};

int createVFS(char * name, unsigned int size);
struct vfs * openVFS(char * name);
void closeVFS(struct vfs * vfs);
int removeVFS(char * name);
int copyIntoVD(char * name, char * sourceFileName, char * destFileName);
int copyFromVD(char * name, char * sourceFileName, char * destFileName);
int removeFromVD(char * name, char *fileName);
int viewFiles(char * name);
int viewInfo(char * name);

char * getDate(time_t time);


int main(int argc, char * argv[]){
  char * vfsName;
  char * func;
  unsigned int error;

  vfsName = argv[2];
  func = argv[1];

  if(strcmp(func, "createVFS") == 0)
	error = createVFS(vfsName, atoi(argv[3]));
  else if(strcmp(func, "copyIntoVD") == 0)
	error = copyIntoVD(vfsName, argv[3], argv[4]);
  else if(strcmp(func, "copyFromVD") == 0)
	error = copyFromVD(vfsName, argv[3], argv[4]);
  else if(strcmp(func, "removeFromVD") == 0)
	error = removeFromVD(vfsName, argv[3]);
  else if(strcmp(func, "viewFiles") == 0)
	error = viewFiles(vfsName);
  else if(strcmp(func, "viewInfo") == 0)
	error = viewInfo(vfsName);
  else if(strcmp(func, "removeVFS") == 0)
	error = removeVFS(vfsName);
  else{
	error = 0;
	printf("Incorrect name of function");
  }

  if(error) printf("Error: Something went wrong try again\n");



  return 0;
}

int createVFS(char * name, unsigned int size){
  FILE * file;
  unsigned int blockAmount;
  struct superblock sb;
  struct dentry * rd;
  struct vfs * vfs;
  char * zeros;
  int * fatTable;
  int i;

  blockAmount = (size - sizeof(sb))/(sizeof(struct dentry) + BLOCK_SIZE + sizeof(int));

  zeros = malloc(size * sizeof(char));
  memset(zeros, 0, sizeof(zeros)); /*wypełnienie tablicy "zeros" zerami*/

  file = fopen(name, "w+");
  if(!file) return 1;

  fwrite(zeros, 1, size, file);

  rd = malloc(sizeof(struct dentry) * blockAmount);
  fatTable = malloc(sizeof(int) * blockAmount);

  sb.diskSize = size;
  strncpy(sb.name, name, MAX_NAME_SIZE);
  sb.fatBlockSize = sizeof(int) * blockAmount;
  sb.rootdirBlockSize = sizeof(struct dentry) * blockAmount;
  sb.blockSize = BLOCK_SIZE;

  for(i=0; i<blockAmount; i++){
    fatTable[i] = -2;
  }

  fseek(file, 0, 0);
  fwrite(&sb, sizeof(sb), 1, file);
  fseek(file, sizeof(sb), 0);
  fwrite(fatTable, sizeof(int), blockAmount, file);

  for(i=0; i<blockAmount; i++){
    strcpy(rd[i].name, "");
    rd[i].size = 0;
    rd[i].firstBlock = -1;
    rd[i].valid = 0;
  }

  vfs = malloc(sizeof(struct vfs));
  vfs->file = file;
  vfs->blockAmount = blockAmount;
  vfs->rd = rd;

  free(fatTable);
  closeVFS(vfs);

  return 0;
}

struct vfs * openVFS(char * name){
  FILE * file;
  unsigned int blockAmount;
  unsigned int size;
  struct superblock sb;
  struct dentry * rd;
  struct vfs * vfs;
  int i;
  int result;

  file = fopen(name, "r+");
  if(!file) return NULL;

  fseek(file, 0, 2);
  size = ftell(file);

  fseek(file, 0, 0);
  result = fread(&sb, sizeof(sb), 1, file);

  if(result == 0 || size < sizeof(struct superblock) || sb.diskSize != size){
  	fclose(file);
  	return NULL;
  }


  blockAmount = (sb.diskSize - sizeof(sb))/(sizeof(struct dentry) + BLOCK_SIZE + sizeof(int));
  rd = malloc(blockAmount * sizeof(struct dentry));

  fseek(file, sizeof(sb)+sizeof(int)*blockAmount, 0);
  result = fread(rd, sizeof(struct dentry), blockAmount, file);

  if(result == 0){
  	free(rd);
  	fclose(file);
  	return NULL;
  }

  vfs = malloc(sizeof(struct vfs));
  vfs->file = file;
  vfs->blockAmount = blockAmount;
  vfs->rd = rd;

  return vfs;
}

void closeVFS(struct vfs * vfs){
  fseek(vfs->file, sizeof(struct superblock)+sizeof(int)*vfs->blockAmount, 0);
  fwrite(vfs->rd, sizeof(struct dentry), vfs->blockAmount, vfs->file);
  fclose(vfs->file);
  free(vfs->rd);
  free(vfs);
  vfs = NULL;
}

int removeVFS(char * name){
  int success;
  success = remove(name);
  if (success != 0) return 1;
  return 0;
}

int copyIntoVD(char * name, char * sourceFileName, char * destFileName){
  FILE * file;
  struct vfs * vfs;

  unsigned int sourceFileSize;
  unsigned int requiredBlocks;
  unsigned int blockIndex;
  unsigned int i;
  unsigned int tmp;
  unsigned int * dentrys;  /* Galezie w ktorych zapiszemy plik */
  unsigned int position; /* Pozycja w pliku na dane */
  int modify;
  int result;
  char data[BLOCK_SIZE];
  int * fatTable;
  time_t tmpCreate;

  vfs = openVFS(name);

  if (!vfs || strcmp(destFileName, "") == 0) return 1; /*zwroc blad jesli vfs lub nazwa puste*/

  modify = -1;
  /*modyfikacja istniejacego pliku*/
  for(i=0; i<vfs->blockAmount; i++){
	   if(vfs->rd[i].valid == 1 && strcmp(vfs->rd[i].name, destFileName) == 0) {
       modify = i;
       break;
     }
  }
  if(modify != -1){
    tmpCreate = vfs->rd[modify].create;
    closeVFS(vfs);
    removeFromVD(name, destFileName);
    vfs = openVFS(name);
    if (!vfs) return 1;
  }

  file = fopen(sourceFileName, "r+");
  if(!file) return 1;

  fatTable = malloc(sizeof(int)*vfs->blockAmount);

  fseek(file, 0, 2);
  sourceFileSize = ftell(file);
  fseek(vfs->file, sizeof(struct superblock), 0);
  result = fread(fatTable, sizeof(int), vfs->blockAmount, vfs->file);
  if(result == 0){
    fclose(file);
    return 1;
  }
  fseek(file, 0, 0);

  requiredBlocks = 1;
  tmp = sourceFileSize;
  while(tmp > BLOCK_SIZE){
  	requiredBlocks++;
  	tmp-=BLOCK_SIZE;
  }

  dentrys = malloc(requiredBlocks * sizeof(unsigned int));

//POPRAWIC!!!!!!!!!
  /* Szukanie wolnych blokow */
  blockIndex = 0;
  for(i=0; i<vfs->blockAmount; i++){
  	if(!vfs->rd[i].valid){
  		dentrys[blockIndex] = i;
  		blockIndex++;
  	}
  	if(blockIndex == requiredBlocks) break;
  }

  if(blockIndex < requiredBlocks){
  	free(dentrys);
  	fclose(file);
	  return 1;
  }

  unsigned int size;

  for(i=0; i<requiredBlocks; i++){
    size = fread(data, 1, sizeof(data), file);
    vfs->rd[dentrys[0]].size += size;
    vfs->rd[dentrys[i]].valid = 1;
    vfs->rd[dentrys[i]].size = size;

  	if(i==0) {
    	strncpy(vfs->rd[dentrys[i]].name, destFileName, MAX_NAME_SIZE);
      vfs->rd[dentrys[i]].firstBlock = dentrys[i];
      vfs->rd[dentrys[i]].modify = time(NULL);
      if(modify == -1) vfs->rd[dentrys[i]].create = vfs->rd[dentrys[i]].modify;
      else vfs->rd[dentrys[i]].create = tmpCreate;

    }

  	if(i < requiredBlocks - 1){
      fatTable[dentrys[i]] = dentrys[i+1];
    }

    if(i == requiredBlocks - 1) {
      fatTable[dentrys[i]] = -1;
    }

  	position = sizeof(struct superblock) + (sizeof(int) + sizeof(struct dentry)) * vfs->blockAmount + BLOCK_SIZE * dentrys[i];

  	fseek(vfs->file, position, 0);
  	fwrite(data, 1, size, vfs->file);

  }

  fseek(vfs->file, sizeof(struct superblock), 0);
  fwrite(fatTable, sizeof(int), vfs->blockAmount, vfs->file);

  free(fatTable);
  free(dentrys);
  fclose(file);
  closeVFS(vfs);
  return 0;

}

int copyFromVD(char * name, char * sourceFileName, char * destFileName){
  FILE * file;
  struct vfs * vfs;
  //struct fat fat;
  unsigned int position;
  unsigned int i;
  int tmp;
  int result;
  char data[BLOCK_SIZE];
  int * fatTable;

  vfs = openVFS(name);

  if (!vfs || strcmp(destFileName, "") == 0) return 1; /*zwroc blad jesli vfs lub nazwa puste*/

  file = fopen(destFileName, "w+");
  if(!file) return 1;

  fatTable = malloc(sizeof(int)*vfs->blockAmount);

  fseek(vfs->file, sizeof(struct superblock), 0);
  result = fread(fatTable, sizeof(int), vfs->blockAmount, vfs->file);
  if(result == 0) return 1;

  tmp = -1;

  for(i=0; i<vfs->blockAmount; i++){
  	if(vfs->rd[i].valid && vfs->rd[i].firstBlock >= 0 && strcmp(vfs->rd[i].name, sourceFileName) == 0){
  		tmp = i;
  		break;
  	}
  }

  if(tmp == -1) {
  	fclose(file);
  	return 1;	/* Nie znalezino szukanego pliku */
  }
  unsigned int size = vfs->rd[tmp].size;
  unsigned int tempSize;

  while(tmp >= 0){
    tempSize = size;
    if(size > BLOCK_SIZE) {
      size-=BLOCK_SIZE;
      tempSize = BLOCK_SIZE;
    }
  	position = sizeof(struct superblock) + (sizeof(int) + sizeof(struct dentry)) * vfs->blockAmount + BLOCK_SIZE * tmp;
  	fseek(vfs->file, position, 0);
  	if(fread(data, 1, tempSize, vfs->file) == 0){
  		fclose(file);
  		return 1;
  	}
  	fwrite(data, 1, tempSize, file);
    tmp = fatTable[tmp];
  }

  free(fatTable);
  fclose(file);
  closeVFS(vfs);
  return 0;
}

int removeFromVD(char * name, char *fileName){
  struct vfs * vfs;
  unsigned int i;
  int tmp;
  int temp;
  int result;
  char data[BLOCK_SIZE];
  int * fatTable;

  vfs = openVFS(name);
  if(!vfs || strcmp(fileName, "") == 0) return 1;

  fatTable = malloc(sizeof(int) * vfs->blockAmount);
  fseek(vfs->file, sizeof(struct superblock), 0);
  result = fread(fatTable, sizeof(int), vfs->blockAmount, vfs->file);
  if(result == 0) return 1;

  tmp = -1;

  for(i=0; i<vfs->blockAmount; i++){
  	if(vfs->rd[i].valid && strcmp(vfs->rd[i].name, fileName) == 0){
  		tmp = i;
  		break;
  	}
  }

  if(tmp == -1) return 1;

  while(tmp >=0){
    temp = fatTable[tmp];
  	vfs->rd[tmp].valid = 0;
  	vfs->rd[tmp].firstBlock = 0;
  	vfs->rd[tmp].size = 0;
  	vfs->rd[tmp].create = 0;
  	vfs->rd[tmp].modify = 0;
	  strcpy(vfs->rd[tmp].name, "");
    fatTable[tmp] = -2;
    tmp = temp;
  }

  free(fatTable);
  closeVFS(vfs);
  return 0;
}

int viewFiles(char * name){
  unsigned int i;
  struct vfs * vfs;
  char * created;
  char * modified;

  vfs = openVFS(name);
  if(!vfs) return 1;

  printf("*********** List of files from \"%s\" ***********\n\n", name);

  for(i=0; i<vfs->blockAmount; i++){
  	if(vfs->rd[i].valid && strcmp(vfs->rd[i].name, "") != 0){
  		printf("%d. File: %s | Size:%d | Created: %s | Modified: ", i+1, vfs->rd[i].name, vfs->rd[i].size, getDate(vfs->rd[i].create));
      printf("%s\n", getDate(vfs->rd[i].modify));
    }
  }

  closeVFS(vfs);
  return 0;
}

int viewInfo(char * name){
  unsigned int i;
  unsigned int unusedBlocks;
  unsigned int diskSize;
  unsigned int adr;
  struct vfs * vfs;
  struct superblock sb;
  int * fatTable;
  int result;

  vfs = openVFS(name);
  if(!vfs) return 1;

  fatTable = malloc(sizeof(int)*vfs->blockAmount);

  fseek(vfs->file, sizeof(struct superblock), 0);
  result = fread(fatTable, sizeof(int), vfs->blockAmount, vfs->file);
  if(!result){
    free(fatTable);
    closeVFS(vfs);
    return 1;
  }

  fseek(vfs->file, 0, 0);
  result = fread(&sb, sizeof(sb), 1, vfs->file);
  if(!result){
    free(fatTable);
    closeVFS(vfs);
    return 1;
  }

  // fseek(vfs->file, 0, 2);
  // diskSize = ftell(vfs->file);

  printf("************ Virtual Disk Info ************\n\n");
  printf("Disk name: %s\n", sb.name);
  printf("Disk size: %d\n", sb.diskSize);
  printf("Number of data blocks: %d (%d B each)\n\n", vfs->blockAmount, sb.blockSize);
  printf("**** Blocks ****\n\n");

  unusedBlocks = 0;
  printf("------------------------------------------------------------------\n");
  printf("Adr: #0 | Type: SB | Size: %ld\n", sizeof(sb));
  printf("------------------------------------------------------------------\n");
  printf("Adr: #%ld | Type: FAT | Size: %d\n", sizeof(sb), sb.fatBlockSize);
    printf("------------------------------------------------------------------\n");
  printf("Adr: #%ld | Type: RD | Size: %d\n", sb.fatBlockSize+sizeof(sb), sb.rootdirBlockSize);
    printf("------------------------------------------------------------------\n");

  adr = sb.fatBlockSize+sizeof(sb)+sb.rootdirBlockSize;

  for(i=0; i<vfs->blockAmount; i++){

    	printf("Adr: #%d | ", adr+i*BLOCK_SIZE);
    	printf("Type: DataBlock | ");
      if(fatTable[i] != -2)
    	 printf("Usage: Used | ");
      else
        printf("Usage: Unused | ");
    	printf("Free Space: %d/%d\n", vfs->rd[i].size>sb.blockSize ? 0 : sb.blockSize-vfs->rd[i].size, sb.blockSize);
      printf("------------------------------------------------------------------\n");

    	if(!(vfs->rd[i].valid)) unusedBlocks++;
  }

  printf("\n**** Disk usage ****\n\n");
  printf("Free blocks number: %d/%d\n", unusedBlocks, vfs->blockAmount);
  printf("Free space for new file: %d B\n\n", unusedBlocks*BLOCK_SIZE);

  free(fatTable);
  closeVFS(vfs);
  return 0;
}

char * getDate(time_t time){
  struct tm * date;
  date = localtime(&time);
  return asctime(date);
}
