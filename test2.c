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
	int i=1;
	while(i<1000000){
		int *a=shmalloc(sizeof(int));
		*a=i++;
		printf("%d\n",*a);
		printMemoryContents();
		shfree(a);
	}	
	unlinkShmMalloc();

    return 0; 
} 
