#ifndef SHMALLOC_H_
#define SHMALLOC_H_
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#define SHMSIZE 512
#define SHMNAME "shmmalloc"
#define SEMNAME "/MALLOCSEMAPHORE"
int thispid;
void *ptr;
struct header{
	int size;
	int pid;
	int free;
	int next;
};

sem_t* sem;
void *shmalloc(int size);
struct header *getFreeMemory(int size);
void initShmMalloc();
void shfree(void *block);
void *shrealloc(void *seg,int size);
void *offsetToPtr(int offset);
int ptrToOffset(void *cmpPtr);
void mergeFreeBlocks();
void printMemoryContents();
void unlinkShmMalloc();

#endif 
