#include "shmalloc.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>  
#include <semaphore.h>








//Functia care initializeaza memoria partajata si seteaza in variablia pidul curent;

void initShmMalloc(){
	int shm_fd;

	if((sem = sem_open(SEMNAME, O_CREAT, (S_IRUSR | S_IWUSR), 1))==SEM_FAILED){
		perror(NULL);
		exit(1);

	}
	shm_fd=shm_open(SHMNAME, O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
	if(shm_fd<0){
		perror(NULL);
		exit(1);
	}
	if(ftruncate(shm_fd,SHMSIZE)==-1){
		perror(NULL);
		shm_unlink(SHMNAME);
		exit(1);
	}
	ptr=mmap(0, SHMSIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
	if(ptr==MAP_FAILED){
		perror(NULL);
		shm_unlink(SHMNAME);
		exit(1);
	}
	thispid=getpid();




}


//Functia de alocare

void *shmalloc(int size){

	if(size<=0) return NULL;
	
	struct header * curr;
	sem_wait(sem);

	if(!*(char*)ptr){
		//Daca nu exista nimic in shm o vom initializa cu 2 elemente. Unul actual, pe care il vom returna, iar daca mai avem loc pentru inca un element il vom adauga si il vom marca ca fiind liber
		struct header *temp=(struct header*)ptr;
		temp->size=size;
		temp->free=0;
		temp->pid=thispid;
		//Daca memoria totala(SHMSIZE) este suficienta pentru a avea 2(sizeof(struct header)+1 bytes) elemente si headerele din lista inlantuita asociate
		if(2*sizeof(struct header)+size<SHMSIZE){
			struct header *temp2=(struct header*)((char*)ptr+sizeof(struct header)+size);
			temp2->size=SHMSIZE-2*sizeof(struct header)-size;
			temp2->free=1;
			temp2->next=-1;
			temp->next=ptrToOffset(temp2);
		}else temp->next=-1;
		sem_post(sem);

		return temp+1;

	}else{
		//Daca memoria a fost deja initializata cautam prima zona de memorie in care putem adauga elementul nostru
		struct header* curr=getFreeMemory(size);
		if(curr){
			curr->free=0;
			curr->pid=thispid;
			//Daca zona de memorie gasita este destul de mare(sizeof(struct header)+1 bytes) incat sa permita impartirea ei in doua elemente o vom face
			//Daca nu vom da zona de memorie chiar daca depasim cu cativa bytes, vom oferi cu maxim sizeof(struct header),adica 16 bytes mai mult
			if(curr->size-size>sizeof(struct header)){
				struct header * tmp=(struct header*)((char*)curr+size+sizeof(struct header));
				tmp->next=curr->next;
				tmp->size=curr->size-size-sizeof(struct header);
				tmp->free=1;
				curr->next=ptrToOffset(tmp);
				curr->size=size;
				
			}
			sem_post(sem);			
			return curr+1;
		}
	}
	return NULL;

}


//Functia de dezalocare
//Marcam segmentul de memorie ca fiind liber
//Apoi functia mergeFreeBlocks() va uni toate blocurile libere de memorie intr-un singur block pentru a evita fragmentarea memoriei
void shfree(void *seg){
	if(seg==NULL) return;
	sem_wait(sem);
	struct header *curr=(struct header*)seg-1;
	curr->free=1;
	mergeFreeBlocks(ptr);
	sem_post(sem);
}


//Functia de realocare
//Cand micsoram zona de memorie verificam daca in spatiul pe care il eliberam avem loc de inca un element(sizeof(struct header)+1 bytes), daca nu avem loc de inca un element lasam dimensiunea aceeasi
//Daca marim zona de memorie verificam daca in continuarea zonei noastre de memorie avem o zona de memorie libera destul de mare astfel incat sa marim zona noastra de memorie o vom face
//Daca marim si nu avem spatiu in continuare, vom aloca o noua zona,copiem si eliberam memoria veche


void *shrealloc(void *seg,int size){

	struct header *tmp=(struct header*)seg-1;
	struct header *curr;

	if(size==tmp->size) return seg;
	if(size<tmp->size){
		if(tmp->size-size>sizeof(struct header)){
			sem_wait(sem);
			curr=(struct header*)((char*)tmp+size+sizeof(struct header));
			curr->size=tmp->size-size-sizeof(struct header);
			curr->next=tmp->next;
			tmp->next=(void*)curr-ptr;
			tmp->size=size;
			curr->free=1;
			mergeFreeBlocks(ptr);
			sem_post(sem);

			
		}

		return tmp+1;
	}else{
	
		struct header *nextBlock=(struct header*)offsetToPtr(tmp->next);
		sem_wait(sem);

		if(nextBlock&&nextBlock->free&&tmp->size+sizeof(struct header)+nextBlock->size>=size){
			if(tmp->size+sizeof(struct header)+nextBlock->size>size+sizeof(struct header)){		
				curr=(struct header*)offsetToPtr((void*)tmp-ptr+size+sizeof(struct header));
				curr->next=nextBlock->next;
				curr->free=1;
				tmp->next=((void*)curr-ptr);
				curr->size=nextBlock->size-(size-tmp->size);
				tmp->size=size;
			}else{
				tmp->size+=sizeof(struct header)+nextBlock->size;
				tmp->next=nextBlock->next;
				

			}
			sem_post(sem);

			return tmp+1;

		}else{
			curr=shmalloc(size);
			if(curr){
				((struct header*)curr-1)->pid=thispid;
				memcpy(curr,seg,tmp->size);
				shfree(seg);
			}
			return curr;
		}
		
	}
	
}

//Functie apelata la iesirea din program care marcheaza toata zonele de memorie din programul curent ca fiind libere si lipeste toate blocurile libere consecutive

void unlinkShmMalloc(){
	struct header *curr=(struct header*)ptr;
	while(curr){		
		if(curr->pid==thispid) curr->free=1;
		curr=(struct header*)(offsetToPtr(curr->next));
	}
	mergeFreeBlocks();
	sem_unlink(SEMNAME);
	shm_unlink(SHMNAME);

}

//Lipim blocurile libere conscutive


void mergeFreeBlocks(){
	struct header *curr=(struct header*)ptr;
	while(curr){
		struct header *tempNext=(struct header*)(offsetToPtr(curr->next));		
		if(tempNext&&curr->free==1&&tempNext->free==1){
			curr->size+=tempNext->size+sizeof(struct header);
			curr->next=tempNext->next;
				
		}else
			curr=(struct header*)(offsetToPtr(curr->next));
		
	}


}


//cautam prima zona de memorie pe care o putem aloca

struct header *getFreeMemory(int size){
	struct header *temp=(struct header *)ptr;
	while(temp){
		if(temp->free==1&&temp->size>=size)
			return temp;
		temp=(struct header*)(offsetToPtr(temp->next));

	};
}


void printMemoryContents(){
	printf("\n");
	struct header *temp=(struct header*)ptr;
	while(temp){
		printf("%d %d %d %d\n",temp->size, temp->free,temp->pid,temp->next);
		temp=offsetToPtr(temp->next);

	}


}




void *offsetToPtr(int offset){
	if(offset==-1) return NULL;
	return ptr+offset;
}


int ptrToOffset(void *cmpPtr){
    if(ptr == NULL) return -1;
    return cmpPtr - ptr;
}



