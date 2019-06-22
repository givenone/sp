/*
*   2017-19428 Seo Junwon Malloc Lab

        <Block Info>
        31                     3  2  1  0
 *      -----------------------------------
 *     | s i   ...   z    e       0  0  a/f -> <header> :: size of the block & "allocated?"" one bit
 *      -----------------------------------
 *     |    
 *     |
 *     |
 *     |
 *     ...
 *     |___________________________________
 *     | s i   ...   z    e       0  0  a/f -> <footer> ::
 *      ------------------------------------
 *   <Free Block>
 * 
 *      31                     3  2  1  0
 *      -----------------------------------
 *     | s i   ...   z    e       0  0  a/f  -> <header>:: size of the block & "allocated?"" one bit
 *      -----------------------------------
 *     |    pointer to its predecessor in Segregated seg_num -> Can access via Macro
 *     |-----------------------------------    
 *     |     pointer to its successor in Segregated seg_num -> Can access via Macro
 *     |
 *     ...
 *     |___________________________________
 *     | s i   ...   z    e       0  0  a/f   -> <footer>
 *      ------------------------------------
 * 
 *      Free seg_num : Segregated seg_num (Array size of "LIST_COUNT")
 *      - partition the block by block size
 * 
 *    <Heap Overview>
 * 
 * begin                                                       end
 *  -----------------------------------------------------------------
 * |free_list| Prologue |        User Blocks             | epilogue |

 * 
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// My additional Macros
#define WSIZE     4          // word and header/footer size (bytes)
#define DSIZE     8          // double word size (bytes)
#define CHUNKSIZE (1<<9)

#define LIST_COUNT 32

#define MAX(x, y) ((x) > (y) ? (x) : (y)) 
#define MIN(x, y) ((x) < (y) ? (x) : (y)) 

// Pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// Read and write a word at address p 
#define GET(p)            (*(unsigned int *)(p))
#define PUT(p, val)       (*(unsigned int *)(p) = (val))

// Store predecessor or successor pointer for free blocks 
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

// Read the size and allocation bit from address p 
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
// Address of block's header and footer 
#define HDRP(ptr) ((char *)(ptr) - WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

// Address of (physically) next and previous blocks 
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE))
#define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE))

// Address of free block's predecessor and successor entries 
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

// Address of free block's predecessor and successor on the segregated seg_num 
#define PRED(ptr) (*(char **)(ptr))
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))

void **free_list; // segregated seg_num
char *heap_list; // for heap consistency check : pointer to the first block 
static void *extend_heap(size_t size);
static void *coalesce(void *ptr);
static void *replace(void *ptr, size_t new_size);
static void insertion(void *ptr, size_t size);
static void deletion(void *ptr);
static int mm_check();

static int mm_check()
{
    // Is every block in the free seg_num marked as free?

    for(int seg_num = 0; seg_num < LIST_COUNT ; seg_num++)
    {
        void *head = free_list[seg_num];
        while(head != NULL)
        {
                        
    // Do the pointers in the free seg_num point to valid free block?

            if(GET_ALLOC(HDRP(head)))
            {
                return -1;
            }

              // Do the pointers in a heap block point to valid heap address?

            if ((PRED(head) != NULL) && ((long)SUCC(PRED(head)) != (long)head)) {
                printf("pointers not consistent:\n");
                return -1;
            }
            // check pointers in heap boundaries
            if (head <= mem_heap_hi() && head >= mem_heap_lo()) {
                printf("Error: pointers not in heap\n");
            }
            
            // check if block falls in the right size class

            head = PRED(head);
        }
    }

    // Are there any contiguous free blocks somehow escaped coalescing
    // Do any allocated blocks overlap?
    int flag = 1;
    for (char *temp = heap_list; GET_SIZE(HDRP(temp)) > 0; temp = NEXT_BLKP(temp)) 
    {
        // Check block is valid
        if ((size_t)ALIGN(*temp) == (size_t)*temp)
        {
            printf("%p is not aligned\n", temp);
            return -1;
        }
        if (((char *)temp >= (heap_list+2*LIST_COUNT*DSIZE)) && GET_SIZE(HDRP(temp)) < 2 * WSIZE)
        {
            return -1;
        }
        if (GET(HDRP(temp)) != GET(FTRP(temp)))
        {
            printf("header does not match footer\n");
            return -1;
        }
        
        // check coalescing
        if (!GET_ALLOC(HDRP(temp)))
        {
            if (flag == 1) 
            {
                printf("consecutive free blocks %p | %p in the heap.\n", PREV_BLKP(temp),temp);
                return -1;
            }
            flag = 1;
        } 
        else 
        {
            flag = 0;
        }
    }
    // Is every free block actually in free seg_num?
    // return 0 if consistent
    return 0;
}
/*
* extend heap using mem_sbrk function
* insert new free block and coalesce.
*/
static void *extend_heap(size_t size)
{
    void *ptr;                   
    size_t new_size = ALIGN(size);
        
    if ((ptr = mem_sbrk(new_size)) == (void *)-1)
        return NULL; // wrapping system call
    
    // Set headers and footer 
    PUT(HDRP(ptr), PACK(new_size, 0));  
    PUT(FTRP(ptr), PACK(new_size, 0));   
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));  // epilogue block
    insertion(ptr, new_size);

    return coalesce(ptr);
}
/*
* insert free block in segregated seg_num
* segregated seg_num is ascending order, seg_num managed in order of size
*/
static void insertion(void *ptr, size_t size) {

    int seg_num = 0;
    while ((seg_num < LIST_COUNT - 1) && (size > 1)) {
        size /= 2;
        seg_num++;
    }
    
    // ascending order and search
    void *head = free_list[seg_num];
    void *prev = NULL;
    while ((head != NULL) && (size > GET_SIZE(HDRP(head)))) {
        prev = head;
        head = PRED(head);
    }

    if (head != NULL) {
        if (prev == NULL) {
            SET_PTR(PRED_PTR(ptr), head);
            SET_PTR(SUCC_PTR(head), ptr);
            SET_PTR(SUCC_PTR(ptr), NULL);
            free_list[seg_num] = ptr;
        } else {
            SET_PTR(PRED_PTR(ptr), head);
            SET_PTR(SUCC_PTR(head), ptr);
            SET_PTR(SUCC_PTR(ptr), prev);
            SET_PTR(PRED_PTR(prev), ptr);
        }
    } else {
        if (prev == NULL) {
            SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), NULL);
            free_list[seg_num] = ptr;
        } else {
            SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), prev);
            SET_PTR(PRED_PTR(prev), ptr);
        }
    }
}

/* delete free block in segregated seg_num
*  free blocks are managed as doubly linked seg_num
*/
static void deletion(void *ptr) {
    
    size_t size = GET_SIZE(HDRP(ptr));
        // Select segregated seg_num 
    int seg_num = 0;
    while ((seg_num < LIST_COUNT - 1) && (size > 1)) {
        seg_num++;
        size /= 2;     
    }
    
    if(PRED(ptr) != NULL && SUCC(ptr) !=NULL)
    {
        SET_PTR(SUCC_PTR(PRED(ptr)), SUCC(ptr));
        SET_PTR(PRED_PTR(SUCC(ptr)), PRED(ptr));
    }
    else if(PRED(ptr)== NULL && SUCC(ptr) !=NULL)
    {
        SET_PTR(PRED_PTR(SUCC(ptr)), NULL);
    }
    else if(PRED(ptr) != NULL && SUCC(ptr) ==NULL)
    {
        SET_PTR(SUCC_PTR(PRED(ptr)), NULL);
        free_list[seg_num] = PRED(ptr);
    }
    else
    {
        free_list[seg_num] = NULL;
    }
}

// coalesce = reference :: textbook

static void *coalesce(void *ptr)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));
    

    if (prev_alloc && next_alloc) {                         // Case 1
        return ptr;
    }
    else if (prev_alloc && !next_alloc) {                   // Case 2
        deletion(ptr);
        deletion(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {                 // Case 3 
        deletion(ptr);
        deletion(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    } else {                                                // Case 4
        deletion(ptr);
        deletion(PREV_BLKP(ptr));
        deletion(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    
    insertion(ptr, size);
    
    return ptr;
}

/* split free block when allocation
*  free block size is equal or larger than newly allocated block
*  So split free block and update free block seg_num
*/
static void *replace(void *ptr, size_t new_size)
{
    size_t ptr_size = GET_SIZE(HDRP(ptr));
    size_t remain = ptr_size - new_size;
    
    deletion(ptr);

    if (remain <= DSIZE * 2) {
        PUT(HDRP(ptr), PACK(ptr_size, 1)); 
        PUT(FTRP(ptr), PACK(ptr_size, 1)); 
    }
    else if(new_size >= 64)
    {
        PUT(HDRP(ptr), PACK(remain,0));
        PUT(FTRP(ptr), PACK(remain,0));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(new_size,1));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(new_size,1));
        insertion(ptr,remain);
        return NEXT_BLKP(ptr);
    }

    else
    {
        PUT(HDRP(ptr), PACK(new_size,1));
        PUT(FTRP(ptr), PACK(new_size,1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(remain,0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(remain,0));
        insertion(NEXT_BLKP(ptr),remain);
        return ptr;
    }
    
    
    return ptr;
}


/*
 * mm_init - initialize the malloc package.
 * Return value : -1 if any problem, 0 otherwise.
 */
int mm_init(void)
{      
    char *heap_start; // Pointer to beginning of heap
    
    free_list = NULL;
    
    // Allocate memory for the initial empty heap 
    if( (heap_start = mem_sbrk(LIST_COUNT * WSIZE)) == NULL)
        return -1;

    free_list = (void**) heap_start; // Utilization diminishes if test is small.
    for(int i=0; i<LIST_COUNT; i++)
    {
        free_list[i] = NULL;
    }
    if( (heap_start = mem_sbrk(4 * WSIZE)) == NULL)
        return -1;

    //heap_list = heap_start;//for consistency check
    PUT(heap_start, 0);                            /* Alignment padding */
    PUT(heap_start + (WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_start + (WSIZE * 2), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_start + (WSIZE * 3), PACK(0, 1));     /* Epilogue header */
    extend_heap(CHUNKSIZE);
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t new_size = MAX(2 * DSIZE, ALIGN(size+DSIZE));      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
   
    // size 0 case
    if (size == 0)
        return NULL;

    // Search for free block in segregated seg_num
    void *ptr = NULL;
    size_t searchsize = new_size;
    for (int seg_num = 0; seg_num < LIST_COUNT; seg_num++) 
    {
        if (((searchsize <= 1) && (free_list[seg_num] != NULL))||(seg_num == LIST_COUNT - 1/*Last seg_num */)) {
            ptr = free_list[seg_num];
            while ((ptr != NULL) && ((new_size > GET_SIZE(HDRP(ptr)))))
            {
                ptr = PRED(ptr);
            }
            if (ptr != NULL)
                break;
        }
        searchsize /= 2;
    }

    if (ptr == NULL) 
    { // extend if no free block
         extendsize = MAX(new_size, CHUNKSIZE);
        if ((ptr = extend_heap(extendsize)) == NULL)
         {
            return NULL;
         }   
    }   
    // replace and divide block
    ptr = replace(ptr, new_size);
    return ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 *
 * returns nothing
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    insertion(ptr, size);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *new_ptr = ptr;    /* Pointer to be returned */
    size_t new_size = size; /* Size of new block */   
    // Ignore size 0 cases
    if (size == 0)
        return NULL;

    new_size = MAX(ALIGN(size+DSIZE), 2*DSIZE);

    if (GET_SIZE(HDRP(ptr)) < new_size) 
    {
        /* Check if next block is a free block or the epilogue block */
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))) 
        {
            int remain = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + GET_SIZE(HDRP(ptr)) - new_size;
            if (remain < 0) 
            {
                if (extend_heap(MAX(-remain, CHUNKSIZE)) == NULL)
                    return NULL;
                remain += MAX(-remain, CHUNKSIZE);
            }
            
            deletion(NEXT_BLKP(ptr)); // delete in the free block seg_num
            // Do not split block
            PUT(HDRP(ptr), PACK(new_size + remain, 1)); 
            PUT(FTRP(ptr), PACK(new_size + remain, 1)); 
        } 
        else 
        {
            new_ptr = mm_malloc(new_size - DSIZE);
            memcpy(new_ptr, ptr, MIN(size, new_size));
            mm_free(ptr);
        }
    }
/*    else if(GET_SIZE(HDRP(ptr))- new_size > 2*WSIZE)
    {
        int oldsize = GET_SIZE(HDRP(ptr));
        PUT(HDRP(ptr), PACK(new_size, 1));
        PUT(FTRP(ptr), PACK(new_size, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize-new_size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(oldsize-new_size, 0));
        mm_free(NEXT_BLKP(ptr));
    }
    */

    return new_ptr;
}
