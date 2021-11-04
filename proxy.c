/* 
 * echoserver.c - A simple connection-based echo server 
 * usage: echoserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "text_analysis.h"
#include "my_cache.h"

#define BUFSIZE 10485760
#define CacheSize 10
#define DefaultMaxAge 3600
#define ReadBits 1024

#if 0
/* 
 * Structs exported from netinet/in.h (for easy reference)
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr; 
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

void MakeKey(char *Host, int Port, char *GET, char* key){
  char portStr[10];
  strcpy(key, "");
  strcpy(portStr, "");
  strcpy(key, Host);
  strcat(key, ":");
  sprintf(portStr, "%d", Port);
  strcat(key, portStr);
  strcat(key, "_");
  strcat(key, GET);
  // printf("Key:%s\n", key);
  
}

int main(int argc, char **argv) {
  int listenfd; /* listening socket */
  int connfd; /* connection socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr, serveraddr1; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char *buf; /* message buffer */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  int sockfd;
  struct hostent *server;

  struct RequestInfo requestInfo;
  struct ResponseInfo responseInfo;
  struct MyCache myCache;
  char *responseInCache;
  char *p;
  char *temp;
  int age = 0;
  char ageLine[200];
  int j;
  int *responseLength;

  buf = (char *) malloc(BUFSIZE * sizeof(char));
  temp = (char *) malloc(BUFSIZE * sizeof(char));
  responseInCache = (char *) malloc(BUFSIZE * sizeof(char));
  responseLength = (int *) malloc (sizeof(int));
  myCache = initializeMyCache(CacheSize);
  char key[200];
  
  printf("proxy is running.\n");
  printf("****************************************************************************************\n");
  /* check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);


  /* socket: create a socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /* build the server's internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET; /* we are using the Internet */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); /* accept reqs to any IP addr */
  serveraddr.sin_port = htons((unsigned short)portno); /* port to listen on */

  /* bind: associate the listening socket with a port */
  if (bind(listenfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* listen: make it a listening socket ready to accept connection requests */
  if (listen(listenfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");

  

  /* main loop: wait for a connection request, echo input line, 
     then close connection. */
  clientlen = sizeof(clientaddr);
  while (1) {
    /* accept: wait for a connection request */
    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (connfd < 0) 
      error("ERROR on accept");
    
    /* gethostbyaddr: determine who sent the message */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("proxy established connection with %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    

    /* read: read input string from the client */
    bzero(buf, BUFSIZE);
    n = read(connfd, buf, BUFSIZE);
    if (n < 0)
      error("ERROR reading from socket");
    printf("proxy received %d bytes:\n%s", n, buf);
    // printf("proxy received %d bytes.\n", n);
    requestInfo = AnalyzeRequest(buf);

    bzero(responseInCache, BUFSIZE);
    bzero(temp, BUFSIZE);
    // find in cache
    
    MakeKey(requestInfo.Host, requestInfo.Port, requestInfo.GET, key);
    getFromMyCache(key, responseInCache, responseLength, myCache);
    if(strcmp(responseInCache, "NA") != 0){
      // printf("responseInCache:\n%s\n", responseInCache);
      // if cached
      // add Age
      age = getAge(key, myCache);
      printf("Age: %d\n", age);
      sprintf(ageLine, "Age: %d\n", age);
      // printf("ageLine: %s\n", ageLine);
      p = strstr(responseInCache, "\r\n");
      p += 2;

      int firstlineLength = p-responseInCache;
      memcpy(temp, responseInCache, firstlineLength);
      // insert age line
      strcat(temp, ageLine);
      // copy remaining part
      // printf("sizeof(temp): %d, sizeof(p): %d, firstlingLength: %d, strlen(ageLine): %d, responseLength: %d\n",
      //   sizeof(temp), sizeof(p), firstlineLength, strlen(ageLine), *responseLength);
      memcpy(temp + firstlineLength + strlen(ageLine), p, *responseLength - firstlineLength);
      bzero(buf, BUFSIZE);
      memcpy(buf, temp, *responseLength + strlen(ageLine));
      // printf("\nThis is from cache:\n%s\n", buf);
      printf("\nThis is from cache:\n");

      n = write(connfd, buf, *responseLength + strlen(ageLine));
    }
    else{
      // forward to another server
      /* socket: create the socket to server*/
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) 
          error("ERROR opening socket");

      /* gethostbyname: get the server's DNS entry */
      server = gethostbyname(requestInfo.Host);
      if (server == NULL) {
          fprintf(stderr,"ERROR, no such host as %s\n", requestInfo.Host);
          exit(0);
      }

      /* build the server's Internet address */
      bzero((char *) &serveraddr1, sizeof(serveraddr1));
      serveraddr1.sin_family = AF_INET;
      bcopy((char *)server->h_addr, 
      (char *)&serveraddr1.sin_addr.s_addr, server->h_length);
      serveraddr1.sin_port = htons(requestInfo.Port);

      /* connect: create a connection with the server */
      if (connect(sockfd, &serveraddr1, sizeof(serveraddr1)) < 0) 
        error("ERROR connecting");

      // forward to server
      n = write(sockfd, buf, n);
      if (n < 0)
        error("ERROR reading from socket");
      // printf("server received %d bytes: %s", n, buf);
      printf("proxy sent %d bytes.\n", n);

      // receive server msg
      bzero(buf, BUFSIZE);
      n = read(sockfd, buf, BUFSIZE);

      // get cache info from response
      responseInfo = AnalyzeResponse(buf);

      j = n;
      // printf("buf:\n%s\n", buf);
      // printf("Content-Length = %d, n = %d, j = %d\n", responseInfo.contentLength, n, j);

      while(j < responseInfo.contentLength){
        // n = read(sockfd, buf+j, responseInfo.contentLength - j);
        n = read(sockfd, buf+j, BUFSIZE);
        if (n < 0) break;
        j += n;
        // printf("buf: %s\n", buf);
        // printf("Content-Length = %d, n = %d, j = %d\n", responseInfo.contentLength, n, j);        
      }

      // printf("buf:\n%s\n", buf);
      // make key as host:port_website
      MakeKey(requestInfo.Host, requestInfo.Port, requestInfo.GET, key);
      if(!responseInfo.hasCacheControl){
        // set default cache
        putIntoMyCache(key, buf, DefaultMaxAge, j, myCache);
      }
      else if(responseInfo.needCache){
        putIntoMyCache(key, buf, responseInfo.maxAge, j, myCache);
      }


      // printf("server received %d bytes: %s", n, buf);
      printf("proxy received %d bytes.\n", n);
      close(sockfd);
      printf("sockfd closed\n");

      printf("\nThis is from server(not from cache):\n");

      n = write(connfd, buf, j);
    }

    // 
    // 
    //
    
    /* write: echo the input string back to the client */
    // n = write(connfd, buf, BUFSIZE);
    printf("proxy sent %d bytes.\n", n);
    if (n < 0) 
      error("ERROR writing to socket");
    close(connfd);
    printf("connfd closed\n");
    printf("****************************************************************************************\n\n");
  }

  free(buf);
  free(responseInCache);
  free(temp);
  free(responseLength);
}

