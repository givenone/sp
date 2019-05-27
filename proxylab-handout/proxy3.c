#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *init_version = "HTTP/1.0\r\n";
static const char *connection = "Connection: close\r\n";
static const char *proxy_connection = "Proxy-Connection: close\r\n";


/* Is it used? */
static const char *accept_str = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";

/* function definitions */
void server_connection(int fd);
int parse_line(char* host, char* filename, char* URI, int* port);
int request_client(rio_t *rio_client, char *str, int client_fd, int port, char* host, char* filename, char *method, char* version);

int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    unsigned int clientlen;
    
    port = atoi(argv[1]);
    printf("juwnon");
    listenfd = Open_listenfd(port);
    struct sockaddr_in client_addr;

    cache_list = init_cache_list();

    // ignore SIGPIPE
    Signal(SIGPIPE, SIG_IGN);
    while(1) {
        
        clientlen = sizeof(struct sockaddr_in);

        pthread_t tid;
		int *connfdp = Malloc(sizeof(int));
		*connfdp = -1;
		*connfdp = Accept(listenfd, (SA *) &client_addr, (socklen_t *)&clientlen);
		Pthread_create(&tid, NULL, (void *)server_connection, (void *)connfdp);
    }
}


/*
1. Cache Hit을 체크 -> Hit이면 바로 Return
    cache_block *ptr;
    	if((ptr = find_cache_block(uri)) != NULL) {
		printf("It is cached\n\n");
		Rio_writen(fd, ptr->response, strlen(ptr->response));
		Rio_writen(fd, ptr->content, ptr->content_size);
		return;
	}
2. Cache Miss인 경우 -> caching
: read_request & read_forward_response 에 caching 부분 추가.
*/

