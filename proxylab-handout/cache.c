#include "cache.h"
#include <stdlib.h>
#include <string.h>

void init()
{
    sum_of_cache = 0;
    rear = front = NULL;
}

cnode *find(char *uri)
{
    cnode *temp = front;

    for(temp = front; temp != NULL; temp = temp->next)
    {
        printf("uri: %s \n THIS IS CACHE", temp-> uri);
        if(!strcmp(temp->uri, uri))
        {   // cache HIT -> should be back

            if(temp == front)
            {
                if(temp->next != NULL)  temp->next->prev = NULL;
                if(rear != NULL)    rear->next = temp;
                temp->prev = rear;
                rear = temp;
            }
            else if(temp == rear); // do nothing
            else
            {
                temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
                temp->prev = rear;
                temp->prev->next = temp;
                rear = temp;
            }
            return temp;
        }
    }
    return NULL;
}
void replacement(int length)
{
    cnode *temp;
    // LRU Policy
	while(front != NULL && sum_of_cache + length > MAX_CACHE_SIZE) {
		sum_of_cache -= front->content_size;
		temp = front->next;
		Free(front);
		front = temp;
	}

	// No blocks in linked list
	if(front == NULL) rear = NULL;
}
void add_cache(char *uri, char *content)
{
    cnode *new_block;

	/* 1.Check the size of the content is too large*/
	if(strlen(content) > MAX_OBJECT_SIZE) return;

	/* 2.Find first whether the following uri is already cached */
	if(find(uri) != NULL) return;

	/* 3.Check the proxy cache is full, then you have to execute cache replacement  policy.*/
	replacement(strlen(content));

	/* 4.Add new cache block into proxy cache */
	new_block = (cnode *) Malloc(sizeof(cnode));

	// Copy data to new block
	strcpy(new_block->uri, uri);
	memcpy(new_block->object, content, strlen(content));
	new_block->content_size = strlen(content);
	new_block->next = NULL;
    new_block->prev = NULL;
	
	// Increase whole cache size being used
	sum_of_cache += strlen(content);
	
	// Add new block to linked list
	if(rear == NULL) front = rear = new_block;
	else {
		rear->next = new_block;
        new_block->prev = rear;
		rear = new_block;
	}

    printf("Cache Addition Finished");
    for(cnode *t = front; t!=NULL; t= t->next)
    {
        printf("%s\n", t->uri);
    }
}