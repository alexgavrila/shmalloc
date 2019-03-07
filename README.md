

# Memory Allocation Function for shared memory

## Introduction
This is a school project for my Operating Systems class. 
Do not use this for anything other than research purposes as it is not thoroughly tested.



## Implemented functions
* shmalloc(size) - get a memory block of size bytes
* shfree(blockPtr) - mark a block as free
* shrealloc(blockPtr,size) - resize a block

## Usage
When using this library you have to call initShmMalloc() at the start of your program to initialize the shared memory variables. Call unlinkShmMalloc() to mark all the blocks used by this program as free and unlink the shared memory file.


## Basic tests done
I wrote 2 test programs. The first one, "test.c", tests that the functions work as intende. The second program, "test2.c", tests if the library can be used by multiple programs. The second program should have at least 2 instances at the same time.


 