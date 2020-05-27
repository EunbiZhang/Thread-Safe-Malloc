#include"my_malloc.h"

#define metadata sizeof(block_t)

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

block_t *header_lock = NULL;
__thread block_t *header_unlock = NULL;


/*--------- Used Mutex Lock to do BF_MALLOC and FREE --------------*/
//Sbrk new memory block and initialize(only for the lock version)
block_t *newBlock(size_t increment){
  block_t *ans = sbrk(increment);
  ans->size = increment;
  ans->next = NULL;
  ans->prev = NULL;
  return ans;
}

//remove some memory block from the free list(only for the lock version)
void* removeBlock(block_t *ptr, size_t realsize){
  size_t oldsize = ptr->size;
  if(oldsize > realsize + metadata){//if the block is big
    block_t *newptr = (void*)ptr + realsize;
    ptr->size = realsize;
    newptr->size = oldsize - realsize;
    if(ptr->prev != NULL){ptr->prev->next = newptr;}
    else{header_lock = newptr;}
    newptr->prev = ptr->prev;
    newptr->next = ptr->next;
    if(ptr->next != NULL){ptr->next->prev = newptr;}
    ptr->prev = NULL;
    ptr->next = NULL;
    return ptr + 1;
  }
  else{//if the block is small
    if(ptr->prev != NULL){ptr->prev->next = ptr->next;}
    else{header_lock = ptr->next;}
    if(ptr->next != NULL){ptr->next->prev = ptr->prev;}
    ptr->prev = NULL;
    ptr->next = NULL;
    return ptr + 1;
  }
}

//merge with right or left(for the lock version and nolock version)
void mergeBlock(block_t *ptr){
  //merge with the right block
  if(ptr->next != NULL && ptr->next == (void*)ptr + ptr->size){
    ptr->size += ptr->next->size;
    if(ptr->next->next != NULL){ptr->next->next->prev = ptr;}
    ptr->next = ptr->next->next;
  }
  //merge with the left block
  if(ptr->prev != NULL && ptr == (void*)ptr->prev + ptr->prev->size){
    ptr->prev->size += ptr->size;
    ptr->prev->next = ptr->next;
    if(ptr->next != NULL){ptr->next->prev = ptr->prev;}
  }
  return;
}

//insert a block to free list and merge(only for the lock version)
void insertBlock(block_t *ptr){
  if(header_lock == NULL){
    header_lock = ptr;
    return;
  }
  if(ptr < header_lock){
    ptr->next = header_lock;
    header_lock->prev = ptr;
    if(header_lock == (void*)ptr + ptr->size){
      ptr->size += header_lock->size;
      ptr->next = header_lock->next;
      if(header_lock->next != NULL){header_lock->next->prev = ptr;}
    }
    header_lock = ptr;
    return;
  }
  block_t *cur = header_lock;
  while(cur != NULL){
    if(cur < ptr && (cur->next == NULL || cur->next > ptr)){
      break;
    }
    if(cur == ptr){
      printf("error!\n");
      return;
    }
    cur = cur->next;
  }
  ptr->next = cur->next;
  ptr->prev = cur;
  cur->next = ptr;
  if(ptr->next != NULL){ptr->next->prev = ptr;}
  mergeBlock(ptr);
  return;
}

//Best-fit Malloc(only for the lock version)
void *bf_malloc(size_t size){
  size_t realsize = size + metadata;
  block_t *ptr = header_lock;
  block_t *ans = NULL;
  size_t min = SIZE_MAX;
  while(ptr != NULL){
    if(ptr->size > realsize && ptr->size < min){
      min = ptr->size;
      ans = ptr;
    }
    if(ptr->size == realsize){
      min = ptr->size;
      ans = ptr;
      break;
    }
    ptr = ptr->next;
  }
  if(ans == NULL){//if no such free block
    ans = newBlock(realsize);
    return ans + 1;
  }
  else{//if such free block exits
    return removeBlock(ans, realsize);
  }
}

//Best-fit Free(only for the lock version)
void bf_free(void *ptr){
  block_t *toFree = ptr - metadata;
  insertBlock(toFree);
  return;
}

//Thread Safe malloc/free: Locking Version 
void *ts_malloc_lock(size_t size){
  pthread_mutex_lock(&lock);
  void* ans = bf_malloc(size);
  pthread_mutex_unlock(&lock);
  return ans;
}
void ts_free_lock(void *ptr){
  pthread_mutex_lock(&lock);
  bf_free(ptr);
  pthread_mutex_unlock(&lock);
}



/*---------------- Used TLS to do BF_MALLOC and FREE ---------------*/
//Sbrk new memory block and initialize(only for the nolock version)
block_t *newBlock_unlock(size_t increment){
  pthread_mutex_lock(&lock);//sbrk syscall's lock
  block_t *ans = sbrk(increment);
  pthread_mutex_unlock(&lock);
  ans->size = increment;
  ans->next = NULL;
  ans->prev = NULL;
  return ans;
}
//remove some memory block from the free list(only for the nolock version)
void* removeBlock_unlock(block_t *ptr, size_t realsize){
  size_t oldsize = ptr->size;
  if(oldsize > realsize + metadata){//if the block is big
    block_t *newptr = (void*)ptr + realsize;
    ptr->size = realsize;
    newptr->size = oldsize - realsize;
    if(ptr->prev != NULL){ptr->prev->next = newptr;}
    else{header_unlock = newptr;}
    newptr->prev = ptr->prev;
    newptr->next = ptr->next;
    if(ptr->next != NULL){ptr->next->prev = newptr;}
    ptr->prev = NULL;
    ptr->next = NULL;
    return ptr + 1;
  }
  else{//if the block is small
    if(ptr->prev != NULL){ptr->prev->next = ptr->next;}
    else{header_unlock = ptr->next;}
    if(ptr->next != NULL){ptr->next->prev = ptr->prev;}
    ptr->prev = NULL;
    ptr->next = NULL;
    return ptr + 1;
  }
}


//insert a block to free list and merge(only for the nolock version)
void insertBlock_unlock(block_t *ptr){
  if(header_unlock == NULL){
    header_unlock = ptr;
    return;
  }
  if(ptr < header_unlock){
    ptr->next = header_unlock;
    header_unlock->prev = ptr;
    if(header_unlock == (void*)ptr + ptr->size){
      ptr->size += header_unlock->size;
      ptr->next = header_unlock->next;
      if(header_unlock->next != NULL){header_unlock->next->prev = ptr;}
    }
    header_unlock = ptr;
    return;
  }
  block_t *cur = header_unlock;
  while(cur != NULL){
    if(cur < ptr && (cur->next == NULL || cur->next > ptr)){
      break;
    }
    if(cur == ptr){
      printf("error!\n");
      return;
    }
    cur = cur->next;
  }
  ptr->next = cur->next;
  ptr->prev = cur;
  cur->next = ptr;
  if(ptr->next != NULL){ptr->next->prev = ptr;}
  mergeBlock(ptr);//shared func with the lock version
  return;
}

//Best-fit Malloc(only for the nolock version)
void *bf_malloc_unlock(size_t size){
  size_t realsize = size + metadata;
  block_t *ptr = header_unlock;
  block_t *ans = NULL;
  size_t min = SIZE_MAX;
  while(ptr != NULL){
    if(ptr->size > realsize && ptr->size < min){
      min = ptr->size;
      ans = ptr;
    }
    if(ptr->size == realsize){
      min = ptr->size;
      ans = ptr;
      break;
    }
    ptr = ptr->next;
  }
  if(ans == NULL){//if no such free block
    ans = newBlock_unlock(realsize);
    return ans + 1;
  }
  else{//if such free block exits
    return removeBlock_unlock(ans, realsize);
  }
}

//Best-fit Free(only for the nolock version)
void bf_free_unlock(void *ptr){
  block_t *toFree = ptr - metadata;
  insertBlock_unlock(toFree);
  return;
}

//Thread Safe malloc/free: Non-Locking Version 
void *ts_malloc_nolock(size_t size){
  return bf_malloc_unlock(size);
}
void ts_free_nolock(void *ptr){
  bf_free_unlock(ptr);
}
