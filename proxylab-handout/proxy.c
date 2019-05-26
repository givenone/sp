#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void server_connection(int fd);
int parse_line(char* host, char* filename, char* URI, int* port);

int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    unsigned int clientlen;
    
    port = atoi(argv[1]);
    printf("juwnon");
    listenfd = Open_listenfd(port);
    struct sockaddr_in client_addr;


    // ignore SIGPIPE
    Signal(SIGPIPE, SIG_IGN);
    while(1) {
        
        clientlen = sizeof(struct sockaddr_in);

        connfd = Accept(listenfd, (SA *) &client_addr, &clientlen);

        //Pthread_create(&tid, NULL, (void *)server_connection, (void *)connfdp);
        server_connection(connfd);
        Close(connfd);

    }
}

void server_connection(int fd)
{
    //Pthread_detach(Pthread_self());

    int port;
    char line[MAXLINE], host[MAXLINE], filename[MAXLINE], METHOD[MAXLINE], URI[MAXLINE], VERSION[MAXLINE];

    rio_t r;

    Rio_readinitb(&r, fd);
    Rio_readlineb(&r, line, MAXLINE);
    sscanf(line, "%s %s %s", METHOD, URI, VERSION);

    printf("%s", line);

    // only get request comes
    if(strcasecmp(METHOD, "GET"))
    {
        //error generation
    }
    // parse
    if(!parse_line(host, filename, URI, &port))
    {
        //error generation
    }

    int clinetfd = Open_clientfd(host, port); // redirect to host
    Rio_writen(clinetfd, line, strlen(line));
    // already read line
    while(strcmp(line, "\r\n"))
    {
        Rio_readlineb(&r, line, MAXLINE);
        Rio_writen(clinetfd, line, strlen(line));
        printf("%s", line);
    }


    
}

int parse_line(char* host, char* filename, char* URI, int* port)
{
    if(sscanf(URI, "http://%[^/]%s", host, filename) != 2) // except "/"
        return 0;

	// substring -> parsing
	if(strstr(host, ":")) {
		if(sscanf(host, "%*[^:]:%d", port) != 1) return 0; // * means "excpet ..."
		if(sscanf(host, "%[^:]", host) != 1) return 0;
	}
	else *port = 80;

	return 1;
}


