#include <stdio.h>
#include "shmalloc.h"

void* trythis(void *arg) {

	int *a=shmalloc(sizeof(int));
	printf("\nThread");
	printMemoryContents();
	*a=50;
	printf("%d",*a);
	shfree(a);
		
	return NULL;
}


  
int main() 
{ 


	initShmMalloc();
	int *a=shmalloc(10);
	*a=10;

	int *b=shmalloc(10);
	int *c=shmalloc(100);	
	int *d=shmalloc(10);	

	printMemoryContents();
	shfree(b);
	shfree(c);
	printMemoryContents();
	b=shmalloc(10);
	b=shrealloc(b,25);
	
	printMemoryContents();
	
	unlinkShmMalloc();
	printMemoryContents();
    return 0; 
} 
