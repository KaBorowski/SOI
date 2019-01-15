#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define BLOCK_SIZE 2048
#define MAX_NAME_SIZE 64


struct superblock{
  unsigned int size;
//  char * name;
  //unsigned int blockSize;
};

struct vfs{
  FILE * file;
  unsigned int inodesAmount;
  //unsigned int blockAmount;
  struct inode * inodes;
};

// struct dentry{
//   char name [MAX_NAME_SIZE];
//   unsigned int size;
// };

struct inode{
  char name [MAX_NAME_SIZE];
  unsigned int size;
  int isUsed;
  int isBeginning;
  int nextNode;
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



int main(int argc, char * argv[]){
  char * vfsName;
  char * func;
  unsigned int error;

  vfsName = argv[1];
  func = argv[2];

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

  if(error) printf("Error: Something went wrong try again");



  return 0;
}

int createVFS(char * name, unsigned int size){
  FILE * file;
  unsigned int inodesAmount;
  struct superblock sb;
  struct inode * inodes;
  struct vfs * vfs;
  char * zeros;
  int i;

  inodesAmount = (size - sizeof(sb))/(sizeof(struct inode) + BLOCK_SIZE);

  zeros = malloc(size * sizeof(char));
  memset(zeros, 0, sizeof(zeros)); /*wypełnienie tablicy "zeros" zerami*/

  file = fopen(name, "w+");
  if(!file) return 1;

  fwrite(zeros, 1, size, file);

  sb.size = size;
  fseek(file, 0, 0);
  fwrite(&sb, sizeof(sb), 1, file);

  inodes = malloc(sizeof(struct inode) * inodesAmount);
  for(i=0; i<inodesAmount; i++){
	inodes[i].isUsed = 0;
	inodes[i].isBeginning = 0;
	inodes[i].nextNode = -1;
	strcpy(inodes[i].name, "");
  }

  vfs = malloc(sizeof(struct vfs));
  vfs->file = file;
  vfs->inodesAmount = inodesAmount;
  vfs->inodes = inodes;

  closeVFS(vfs);

  return 0;
}

struct vfs * openVFS(char * name){
  FILE * file;
  unsigned int inodesAmount;
  unsigned int size;
  struct superblock sb;
  struct inode * inodes;
  struct vfs * vfs;
  int i;
  int result;

  file = fopen(name, "r+");
  if(!file) return NULL;

  fseek(file, 0, 2);
  size = ftell(file);

  fseek(file, 0, 0);
  result = fread(&sb, sizeof(sb), 1, file);

  if(result == 0 || size < sizeof(struct superblock) || sb.size != size){
	fclose(file);
	return NULL;
  }

  inodesAmount = (sb.size - sizeof(sb))/(sizeof(struct inode) + BLOCK_SIZE);
  inodes = malloc(inodesAmount * sizeof(struct inode));

  result = fread(inodes, sizeof(struct inode), inodesAmount, file);

  if(result == 0){
	free(inodes);
	fclose(file);
	return NULL;
  }

  vfs = malloc(sizeof(struct vfs));
  vfs->file = file;
  vfs->inodesAmount = inodesAmount;
  vfs->inodes = inodes;

  return vfs;
}

void closeVFS(struct vfs * vfs){
  fseek(vfs->file, sizeof(struct superblock), 0);
  fwrite(vfs->inodes, sizeof(struct inode), vfs->inodesAmount, vfs->file);
  fclose(vfs->file);
  free(vfs->inodes);
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
  unsigned int requiredInodes;
  unsigned int inodeIndex;
  unsigned int i;
  unsigned int tmp;
  unsigned int * nodes;  /* Galezie w ktorych zapiszemy plik */
  unsigned int position; /* Pozycja w pliku na dane */
  int result;
  char data[BLOCK_SIZE];

  vfs = openVFS(name);

  if (!vfs || strcmp(destFileName, "") == 0) return 1; /*zwroc blad jesli vfs lub nazwa puste*/

  /*zwroc blad jesli istnieje plik o takiej nazwie*/
  for(i=0; i<vfs->inodesAmount; i++){
	if(vfs->inodes[i].isUsed == 1 && strcmp(vfs->inodes[i].name, destFileName) == 0) return 1;
  }

  file = fopen(sourceFileName, "r+");
  if(!file) return 1;

  fseek(file, 0, 2);
  sourceFileSize = ftell(file);
  fseek(file, 0, 0);

  requiredInodes = 1;
  tmp = sourceFileSize;
  while(tmp > BLOCK_SIZE){
	requiredInodes++;
	tmp-=BLOCK_SIZE;
  }

  nodes = malloc(requiredInodes * sizeof(unsigned int));

  /* Szukanie wolnych galezi */
  inodeIndex = 0;
  for(i=0; i<vfs->inodesAmount; i++){
	if(!vfs->inodes[i].isUsed){
		nodes[inodeIndex] = i;
		inodeIndex++;
	}
	if(inodeIndex == requiredInodes) break;
  }

  if(inodeIndex < requiredInodes){
	free(nodes);
	fclose(file);
	return 1;
  }

  for(i=0; i<requiredInodes; i++){
	vfs->inodes[nodes[i]].isUsed = 1;
	vfs->inodes[nodes[i]].size = fread(data, 1, sizeof(data), file);
	strncpy(vfs->inodes[nodes[i]].name, destFileName, MAX_NAME_SIZE);

	if(i==0) vfs->inodes[nodes[i]].isBeginning = 1;

	if(i < requiredInodes - 1) vfs->inodes[nodes[i]].nextNode = nodes[i+1];

	position = sizeof(struct superblock) + sizeof(struct inode) * vfs->inodesAmount + BLOCK_SIZE * nodes[i];

	fseek(vfs->file, position, 0);
	fwrite(data, 1, vfs->inodes[nodes[i]].size, vfs->file);

  }

  free(nodes);
  fclose(file);
  closeVFS(vfs);
  return 0;

}

int copyFromVD(char * name, char * sourceFileName, char * destFileName){
  FILE * file;
  struct vfs * vfs;
  unsigned int position;
  unsigned int i;
  int tmp;
  int temp;
  char data[BLOCK_SIZE];

  vfs = openVFS(name);

  if (!vfs || strcmp(destFileName, "") == 0) return 1; /*zwroc blad jesli vfs lub nazwa puste*/

  file = fopen(destFileName, "w+");
  if(!file) return 1;

  tmp = -1;

  for(i=0; i<vfs->inodesAmount; i++){
	if(vfs->inodes[i].isUsed && vfs->inodes[i].isBeginning && strcmp(vfs->inodes[i].name, sourceFileName) == 0){
		tmp = i;
		break;
	}
  }

  if(tmp == -1) {
	fclose(file);
	return 1;	/* Nie znalezino szukanego pliku */
  }

  while(tmp != -1){
	position = sizeof(struct superblock) + vfs->inodesAmount * sizeof(struct inode) + BLOCK_SIZE * tmp;
	fseek(vfs->file, position, 0);
	if(fread(data, 1, vfs->inodes[tmp].size, vfs->file) == 0){
		fclose(file);
		return 1;
	}
	fwrite(data, 1, vfs->inodes[tmp].size, file);
	tmp = vfs->inodes[tmp].nextNode;
  }

  fclose(file);
  closeVFS(vfs);
  return 0;
}

int removeFromVD(char * name, char *fileName){
  struct vfs * vfs;
  unsigned int i;
  int tmp;
  int temp;
  char data[BLOCK_SIZE];

  vfs = openVFS(name);
  if(!vfs) return 1;

  tmp = -1;

  for(i=0; i<vfs->inodesAmount; i++){
	if(vfs->inodes[i].isUsed && vfs->inodes[i].isBeginning && strcmp(vfs->inodes[i].name, fileName) == 0){
		tmp = i;
		break;
	}
  }

  if(tmp == -1) return 1;

  while(tmp != -1){
	temp = vfs->inodes[tmp].nextNode;
	vfs->inodes[tmp].isUsed = 0;
	vfs->inodes[tmp].isBeginning = 0;
	vfs->inodes[tmp].size = 0;
	vfs->inodes[tmp].nextNode = -1;
	strcpy(vfs->inodes[tmp].name, "");
    	tmp = temp;
  }

  closeVFS(vfs);
  return 0;
}

int viewFiles(char * name){
  unsigned int i;
  struct vfs * vfs;

  vfs = openVFS(name);
  if(!vfs) return 1;

  printf("*********** List of files from \"%s\" ***********\n\n", name);

  for(i=0; i<vfs->inodesAmount; i++){
	if(vfs->inodes[i].isUsed && vfs->inodes[i].isBeginning)
		printf("Pos: %d. File: %s\n", i, vfs->inodes[i].name);
  }

  closeVFS(vfs);
  return 0;
}

int viewInfo(char * name){
  unsigned int i;
  unsigned int unusedNodes;
  unsigned int diskSize;
  struct vfs * vfs;

  vfs = openVFS(name);
  if(!vfs) return 1;

  fseek(vfs->file, 0, 2);
  diskSize = ftell(vfs->file);

  printf("************ Virtual Disk Info ************\n\n");
  printf("Disk size: %d\n\n", diskSize);
  printf("Number of data blocks: %d (%d B each)\n\n", vfs->inodesAmount, BLOCK_SIZE);
  printf("**** Inodes ****\n\n");

  unusedNodes = 0;

  for(i=0; i<vfs->inodesAmount; i++){
	printf("ID: %d | ", i);
	printf("Usage: %s | ", vfs->inodes[i].isUsed ? "Used" : "Unused");
	printf("Beginning of file: %s | ", vfs->inodes[i].isBeginning ? "Yes" : "No");
	printf("Name: %s | ", vfs->inodes[i].name);
	printf("Size: %d | ", vfs->inodes[i].size);
	if(vfs->inodes[i].nextNode != -1)
		printf("Next node: %d", vfs->inodes[i].nextNode);

	if(!(vfs->inodes[i].isUsed)) unusedNodes++;
	printf("\n");
  }

  printf("\n**** Disk usage ****\n\n");
  printf("Free blocks number: %d/%d\n", unusedNodes, vfs->inodesAmount);
  printf("Free space: %d B\n\n", unusedNodes*BLOCK_SIZE);

  closeVFS(vfs);
  return 0;
}
