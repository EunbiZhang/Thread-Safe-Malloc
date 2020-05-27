#ifndef __MY_MALLOC_H__
#define __MY_MALLOC_H__
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/time.h>
#include<stdbool.h>
#include<pthread.h>
#include<stdint.h>

//Use link-list to store free memory regions
struct block_tag{
  size_t size;
  struct block_tag *next;
  struct block_tag *prev;
};typedef struct block_tag block_t;

//Thread Safe malloc/free: locking version 
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version 
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

#endif
