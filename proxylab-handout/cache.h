#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// Size of whole cache currently used
int sum_of_cache;

// Contruct linked list using cache_block structure
typedef struct cnode {
	char uri[MAXLINE];
	char object[MAX_OBJECT_SIZE];
	int content_size;
	struct cnode *next;
    struct cnode *prev;
} cnode;

// For using LRU Policy
cnode *rear, *front;

/*function prototypes*/
void init();
cnode *find(char *uri);
void replacement(int length);
void add_cache(char *uri,  char *content);
