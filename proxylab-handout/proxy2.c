#include <stdio.h>
#include "csapp.h"

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
void server_connection(void *vargp);
int parse_line(char* host, char* filename, char* URI, char* port);
int forward_to_server(char *host, char* port, int *server_fd, char *request_str);
int read_and_forward_response(int server_fd, int client_fd);
int request_client(rio_t *rio_client, char *str, int client_fd, char* port, char* host, char* filename, char *method, char* version);

int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    unsigned int clientlen;
    
    listenfd = Open_listenfd(argv[1]);
    //printf("2 : ok now on \n");
    struct sockaddr_in client_addr;

    Signal(SIGPIPE, SIG_IGN);

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

//Pthread_detach(Pthread_self());를 server connection에 추가해주어야 함.

void server_connection(void *vargp)
{
    Pthread_detach(Pthread_self());

    int clientfd = *((int *) vargp);
    char port[MAXLINE];
    char line[MAXLINE], host[MAXLINE], filename[MAXLINE], method[MAXLINE], URI[MAXLINE], version[MAXLINE];
    char client_str[MAXLINE];
    rio_t r;

    Rio_readinitb(&r, clientfd);
    Rio_readlineb(&r, line, MAXLINE);
    sscanf(line, "%s %s %s", method, URI, version);

    printf("request : %s\n", line);
    printf("method : %s\n", method);
    printf("uri : %s\n", URI);
    printf("http version : %s\n", version);

    // only get request comes
    if(strcasecmp(method, "GET"))
    {
        //error generation
    }
    // parse
    if(!parse_line(host, filename, URI, port))
    {
        //error generation
    }
    printf("parsing : ok now on \n");
    printf("host : %s\n", host);
    printf("filename : %s\n", filename);
    //printf("%s\n", URI);
    printf("port : %s\n", port);

    //int clientfd = Open_clientfd(host, port); // redirect to host
    request_client(&r, client_str, clientfd, port, host, filename, method, version);
    printf("read request : ok now on \n");
    printf("%s \n", client_str);

    int serverfd;
    forward_to_server(host, port, &serverfd, client_str);
    printf("forwarding ok : ok now on \n");

    read_and_forward_response(serverfd, clientfd);

    printf("forward to client : ok now on \n");
    if(clientfd >=0)
		Close(clientfd);
		
	if(serverfd >=0)
		Close(serverfd);
/*
    Rio_writen(clinetfd, line, strlen(line));
    // already read line
    while(strcmp(line, "\r\n"))
    {
        Rio_readlineb(&r, line, MAXLINE);
        Rio_writen(clinetfd, line, strlen(line));
        printf("%s", line);
    }
*/
    
}


/*
part 1 :: sequential

1. Read Request from client
2. parse
3. forward to server
4. forward response to client
5. close all fd.
*/

int request_client(rio_t *rio_client, char *str, int client_fd, char* port, char* host,
 char* filename, char *method, char* version)
{
    char tmpstr[MAXBUF];
    char tmpport[MAXBUF];
	
    if (strstr(method, "GET")) {
		strcpy(str, method);
		strcat(str, " ");
		strcat(str, filename);
		strcat(str, " ");
		strcat(str, init_version);
    

        if(strlen(host))
	    {
		    strcpy(tmpstr, "Host: ");
		    strcat(tmpstr, host);
		    strcat(tmpstr, ":");
		    strcat(tmpstr, port);
		    strcat(tmpstr, "\r\n");
		    strcat(str, tmpstr);
	    }
		
		strcat(str, user_agent_hdr);
		strcat(str, accept_str);
		strcat(str, accept_encoding);
		strcat(str, connection);
		strcat(str, proxy_connection);
		
		while(Rio_readlineb(rio_client, tmpstr, MAXBUF) > 0) {
			if (!strcmp(tmpstr, "\r\n")){
				strcat(str,"\r\n");
				break;
			}/*
			else if(strstr(tmpstr, "User-Agent:") || strstr(tmpstr, "Accept:") ||
				strstr(tmpstr, "Accept-Encoding:") || strstr(tmpstr, "Connection:") ||
				strstr(tmpstr, "Proxy Connection:") || strstr(tmpstr, "Cookie:"))
				continue;
			else if (strstr(tmpstr, "Host:")) {
				if (!strlen(host)) {
					strcpy(tmpstr, "Host: ");
					strcat(tmpstr, host);
					strcat(tmpstr, ":");
		            strcat(tmpstr, port);
					strcat(tmpstr, "\r\n");
					strcat(str, tmpstr);
				}
			}*/
			else
				strcat(str, tmpstr);
		}
		
		return 0;
    }
	return 1;    
}

int read_and_forward_response(int server_fd, int client_fd) {
		
	
	char tmp_str[MAXBUF], content[MAX_OBJECT_SIZE];
	unsigned int size = 0, len = 0, cache_size = 0;
	int valid_size = 1;
	
	content[0] = '\0';

	rio_t rio_server;
	Rio_readinitb(&rio_server, server_fd);
	
    while ((len = Rio_readlineb(&rio_server, tmp_str, MAXLINE)) != 0) {

        //printf("Fd = %d, Sum = %d, n = %d\n", mio_internet.mio_fd, sum, n);
        size += len;
        if (size <= MAX_OBJECT_SIZE)
            strcat(content, tmp_str);
        Rio_writen(client_fd, tmp_str, len);
	}

	return 0;
}

int forward_to_server(char *host, char *port, int *server_fd, char *request_str)  
{
	*server_fd = Open_clientfd(host, port);
	
	if (*server_fd < 0) {
		if (*server_fd == -1)
			return -1;
		else 
			return -2;
	}
    Rio_writen(*server_fd, request_str, strlen(request_str));

	return 0;
}

int parse_line(char* host, char* filename, char* URI, char* port)
{
    if(sscanf(URI, "http://%[^/]%s", host, filename) != 2) // except "/"
        return 0;

	// substring -> parsing
	if(strstr(host, ":")) {
		if(sscanf(host, "%*[^:]:%s", port) != 1) return 0; // * means "excpet ..."
		if(sscanf(host, "%[^:]", host) != 1) return 0;
	}
	else strcpy(port, "80");
	return 1;
}


