//------------------------------------------------------------------------------
//
// memtrace
//
// trace calls to the dynamic memory manager
//
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memlog.h>
#include <memlist.h>
#include "callinfo.h"

//
// function pointers to stdlib's memory management functions
//
static void *(*mallocp)(size_t size) = NULL;
static void (*freep)(void *ptr) = NULL;
static void *(*callocp)(size_t nmemb, size_t size);
static void *(*reallocp)(void *ptr, size_t size);

//
// statistics & other global variables
//
static unsigned long n_malloc  = 0;
static unsigned long n_calloc  = 0;
static unsigned long n_realloc = 0;
static unsigned long n_allocb  = 0;
static unsigned long n_freeb   = 0;
static item *list = NULL;

//
// init - this function is called once when the shared library is loaded
//
__attribute__((constructor))
void init(void)
{
  char *error;

  LOG_START();

  // initialize a new list to keep track of all memory (de-)allocations
  // (not needed for part 1)
  list = new_list();

  // ...
}

//
// fini - this function is called once when the shared library is unloaded
//
__attribute__((destructor))
void fini(void)
{
  // ...

   LOG_STATISTICS(n_allocb,
  n_allocb / (n_malloc + n_calloc + n_realloc), n_freeb);
  

//  LOG_BLOCK(ptr, size, cnt, fn, ofs)

    int  bool_for_nonfreestart = 1;
  item *prev = list;

  while (prev != NULL) {
    if(prev->cnt != 0)
    {
        if(bool_for_nonfreestart)
        {
            bool_for_nonfreestart = 0;
              LOG_NONFREED_START();
        }
      LOG_BLOCK(prev->ptr, prev->size, prev->cnt, prev->fname, prev->ofs);
    }
    prev = prev->next;
  }

  LOG_STOP();

  // free list (not needed for part 1)
  free_list(list);
}


void *malloc(size_t size)
{

  mallocp = dlsym(RTLD_NEXT, "malloc");
 
  void *ptr = mallocp(size);

  LOG_MALLOC((int) size, ptr);

  alloc(list, ptr, size);
  
  n_malloc++;
  n_allocb += size;

  return ptr;
}

void *calloc(size_t nmemb, size_t size)
{


  callocp = dlsym(RTLD_NEXT, "calloc");

  void *ptr = callocp(nmemb, size);
  LOG_CALLOC((int) nmemb, (int) size, ptr);

  alloc(list, ptr, size * nmemb);

  n_calloc++;
  n_allocb += ((int) size * nmemb);

  return ptr;
}

void *realloc(void *ptr, size_t size)
{
  reallocp = dlsym(RTLD_NEXT, "realloc");

    int old_size = -1;
  
  item *prev = list;
  while(prev != NULL)
  {
      if(prev -> ptr == ptr)
      {
          old_size = prev->size;
          break;
      }
      prev = prev->next;
  }

    void *newptr;
    if(old_size == -1)
    {
        newptr = reallocp(NULL, size);
    }
    else if(old_size - size >= 0 )
    {
        n_freeb += old_size - size;
        newptr = reallocp(ptr, size);
    }
    else
    {      
        newptr = reallocp(ptr, size);
    }

  LOG_REALLOC(ptr, size, newptr);

  dealloc(list, ptr);
  alloc(list, newptr, size);

  
  n_realloc++;
  n_allocb += (size);

  return newptr;
}

void free(void *ptr)
{

  if(!ptr)
  {
    return;
  }    

  freep = dlsym(RTLD_NEXT, "free");
  item *temp = find(list,ptr);

  LOG_FREE(ptr);
  if(temp != NULL)
  {
    if(temp->cnt == 0)
    {
      LOG_DOUBLE_FREE();

    }
    else
    {
      freep(ptr);

      dealloc(list, ptr);
      n_freeb += temp -> size;
    }
  }
  else
  {
    LOG_ILL_FREE();
  }
  


  //freep = NULL;
}