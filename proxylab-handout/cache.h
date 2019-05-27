nclude "csapp.h"

#define MAX_CONTENT_SIZE 200000 // MAX content size for proxy server: 200kb
#define MAX_OBJECT_SIZE 1000000 // MAX cache size for proxy server: 1MB
#define MAX_URL_SIZE 1024
#define MAX_RESP_SIZE 1024

// Contruct linked list using cache_block structure
typedef struct cache_block {
		/* 
		 * Cache block needs to contain 
		 * 1. URL (recommend to use key value)
		 * 2. Response
		 * 3. contents
		 * 4. size of contents
		 * 5. pointer to next cache_block
		 */
	char url[MAX_URL_SIZE];
	char response[MAX_RESP_SIZE];
	char content[MAX_CONTENT_SIZE];
	int content_size;
	struct cache_block *next;
} cache_block;

// Rear points to recently added cache_block
// Front points to lastly added cache_block
cache_block *rear, *front;

// Size of whole cache currently used
int object_size;

/* Cache function prototypes*/
cache_block *find_cache_block(char *uri);
void cache_replacement_policy(int contentLength);
void add_cache_block(char *uri, char *response, char *content, int contentLength);
