/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE     4          // word and header/footer size (bytes)
#define DSIZE     8          // double word size (bytes)
#define INITCHUNKSIZE (1<<6)
#define CHUNKSIZE (1<<12)

#define LIST    20      
#define REALLOC_BUFFER  (1<<7)    

#define MAX(x, y) ((x) > (y) ? (x) : (y)) 
#define MIN(x, y) ((x) < (y) ? (x) : (y)) 

// Pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// Read and write a word at address p 
#define GET(p)            (*(unsigned int *)(p))
#define PUT(p, val)       (*(unsigned int *)(p) = (val) | GET_TAG(p))
#define PUT_NOTAG(p, val) (*(unsigned int *)(p) = (val))

// Store predecessor or successor pointer for free blocks 
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

// Read the size and allocation bit from address p 
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_TAG(p)   (GET(p) & 0x2)
#define SET_RATAG(p)   (GET(p) |= 0x2)
#define REMOVE_RATAG(p) (GET(p) &= ~0x2)

// Address of block's header and footer 
#define HDRP(ptr) ((char *)(ptr) - WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

// Address of  next and previous blocks 
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE))
#define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE))

// Address of free block's predecessor and successor entries 
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

// Address of free block's predecessor and successor on the segregated list 
#define PRED(ptr) (*(char **)(ptr))
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))

// End of my additional macros

// Global var : free list
void *free_list;


static void *extend_heap(size_t size);
static void insert_node(void *ptr, size_t size);
static void delete_node(void *ptr);
static void *coalesce(void *ptr);
static void *resize(void *ptr, size_t size);

static void *extend_heap(size_t size)
{
    void* ptr = NULL;
    size_t ex_size = ALIGN(size);
    if((ptr = mem_sbrk(ex_size)) == (void *)-1)
    { // error
        return NULL;
    }
    // header & footer
    PUT_NOTAG(HDRP(ptr), PACK(ex_size, 0)); // size & allocated bit : 0 (not allocated)
    PUT_NOTAG(FTRP(ptr), PACK(ex_size, 0));
    PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
    insert_node(ptr, ex_size);

    return coalesce(ptr);
}

static void insert_node(void *ptr, size_t size)
{
    // in ascending order

    void *head = free_list, *prev = NULL;
    while( (head != NULL) && (size > GET_SIZE(HDRP(head))) )
    {
        prev = head;
        head = PRED(head);
    }

    // insert in doubly linked list

    if(head != NULL && prev != NULL)
    {
        SET_PTR(PRED_PTR(ptr), head);
        SET_PTR(SUCC_PTR(head), ptr);
        SET_PTR(SUCC_PTR(ptr), prev);
        SET_PTR(PRED_PTR(prev), ptr);
    }
    else if(head != NULL && prev == NULL)
    {// first element
        SET_PTR(PRED_PTR(ptr), head);
        SET_PTR(SUCC_PTR(head), ptr);
        SET_PTR(SUCC_PTR(ptr), NULL);
        free_list = ptr;
    }
    else if(head == NULL && prev != NULL)
    {// last element
        SET_PTR(PRED_PTR(ptr), NULL);
        SET_PTR(SUCC_PTR(ptr), prev);
        SET_PTR(PRED_PTR(prev), ptr);
    }
    else
    {// very first element
        SET_PTR(PRED_PTR(ptr), NULL);
        SET_PTR(SUCC_PTR(ptr), NULL);
        free_list = ptr;
    }
    
}

static void delete_node(void *ptr)
{ // delete in the doubly linked list
  //  size_t size = GET_SIZE(HDRP(ptr));
    if(SUCC(ptr) == NULL)
    {// first element
        if(PRED(ptr) != NULL)
        {
            SET_PTR(SUCC_PTR(PRED(ptr)), NULL);
        }
        free_list = PRED(ptr);
    }
    else
    {
        if(PRED(ptr) != NULL)
        {
            char* temp1 = SUCC_PTR(PRED(ptr));
            char* temp2 = SUCC(ptr);
            SET_PTR(temp1, temp2);
            SET_PTR(PRED_PTR(SUCC(ptr)), PRED(ptr));
        }
        else
        {
            SET_PTR(PRED_PTR(SUCC(ptr)), NULL);
        }
        
    }
}

static void *coalesce(void *ptr)
{// coalesce the free block -> 4 cases

    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));
    

    // Do not coalesce with previous block if the previous block is tagged with Reallocation tag
    if (GET_TAG(HDRP(PREV_BLKP(ptr))))
        prev_alloc = 1;

    if (prev_alloc && next_alloc) {                         // Case 1
        return ptr;
    }
    else if (prev_alloc && !next_alloc) {                   // Case 2
        delete_node(ptr);
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {                 // Case 3 
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        ptr = PREV_BLKP(ptr);
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(ptr), PACK(size, 0));
        
    } else {                                                // Case 4
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    
    insert_node(ptr, size);
    
    return ptr;
}

static void *resize(void *ptr, size_t size)
{// resize the alloacted block and manage the free list

    size_t original_size = GET_SIZE(HDRP(ptr));
    size_t remain = original_size - size;
    delete_node(ptr);

    if (remain <= DSIZE * 2) 
    {
        // Do not split block 
        PUT(HDRP(ptr), PACK(original_size, 1)); 
        PUT(FTRP(ptr), PACK(original_size, 1)); 
    }
    
    else if (size >= 100) {
        // Split block
        PUT(HDRP(ptr), PACK(remain, 0));
        PUT(FTRP(ptr), PACK(remain, 0));
        PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(size, 1));
        PUT_NOTAG(FTRP(NEXT_BLKP(ptr)), PACK(size, 1));
        insert_node(ptr, remain);
        return NEXT_BLKP(ptr);
    }
    
    else {
        // Split block
        PUT(HDRP(ptr), PACK(size, 1)); 
        PUT(FTRP(ptr), PACK(size, 1)); 
        PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(remain, 0)); 
        PUT_NOTAG(FTRP(NEXT_BLKP(ptr)), PACK(remain, 0)); 
        insert_node(NEXT_BLKP(ptr), remain);
    }
    return ptr;

}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    free_list = NULL;
    char *heap = mem_sbrk(4*WSIZE);

    PUT_NOTAG(heap, 0);
    PUT_NOTAG(heap + (WSIZE), PACK(DSIZE, 1));
    PUT_NOTAG(heap + (WSIZE * 2), PACK(DSIZE, 1));
    PUT_NOTAG(heap + (WSIZE * 3), PACK(0, 1));

    extend_heap(INITCHUNKSIZE);

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
   if(size == 0)
   {
       return NULL;
   }
    int newsize = ALIGN(size + DSIZE);

   if(size <= DSIZE)
   {
       newsize = 2 * DSIZE;
   }

   void *ptr = free_list;

    while((ptr != NULL) && ( (newsize > GET_SIZE(HDRP(ptr))) || (GET_TAG(HDRP(ptr)))/*reallocation*/ ) )
    {
        ptr = PRED(ptr);
    }
   if(ptr == NULL)
   {// No free Block -> extend heap
        if( (ptr = extend_heap(MAX(newsize, CHUNKSIZE)) ) == NULL) 
        {
            return NULL;
        }
   } 
   ptr = resize(ptr, newsize);
   return ptr; //
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    REMOVE_RATAG(HDRP(NEXT_BLKP(ptr)));
    PUT(HDRP(ptr), PACK(size,0));
    PUT(FTRP(ptr), PACK(size,0));

    insert_node(ptr, size);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{


    void *new_ptr = ptr;    /* Pointer to be returned */
    size_t new_size; /* Size of new block */
    int remainder;          /* Adequacy of block sizes */
    int extendsize;         /* Size of heap extension */
    int block_buffer;       /* Size of block buffer */
    
    // Ignore size 0 cases
    if (size == 0)
        return NULL;
    
    // Align block size
    if (size <= DSIZE) {
        new_size = 2 * DSIZE;
    } else {
        new_size = ALIGN(size+DSIZE);
    }
    
    /* Add overhead requirements to block size */
    new_size += REALLOC_BUFFER;
    
    /* Calculate block buffer */
    block_buffer = GET_SIZE(HDRP(ptr)) - new_size;
    
    /* Allocate more space if overhead falls below the minimum */
    if (block_buffer < 0) {
        /* Check if next block is a free block or the epilogue block */
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))) 
        {
            remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - new_size;
            if (remainder < 0) {
                extendsize = MAX(-remainder, CHUNKSIZE);
                if (extend_heap(extendsize) == NULL)
                    return NULL;
                remainder += extendsize;
            }
            
            delete_node(NEXT_BLKP(ptr));
            
            // Do not split block
            PUT_NOTAG(HDRP(ptr), PACK(new_size + remainder, 1)); 
            PUT_NOTAG(FTRP(ptr), PACK(new_size + remainder, 1)); 
        } else {
            new_ptr = mm_malloc(new_size - DSIZE);
            memcpy(new_ptr, ptr, MIN(size, new_size));
            mm_free(ptr);
        }
        block_buffer = GET_SIZE(HDRP(new_ptr)) - new_size;
    }
    
    // Tag the next block if block overhead drops below twice the overhead 
    if (block_buffer < 2 * REALLOC_BUFFER)
        SET_RATAG(HDRP(NEXT_BLKP(new_ptr)));
    
    // Return the reallocated block 
    return new_ptr;
  /*  
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
    */

}














