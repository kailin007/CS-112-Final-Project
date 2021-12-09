#include "Get_request.h"

#define BUFSIZE 5242880
#define ReqBuf 3000
#define DefaultMaxAge 600
#define h_addr h_addr_list[0]

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

void rewrite_header(struct RequestInfo *requestInfo, char* rewrite_req)
{
    memcpy(rewrite_req,"GET ",4);
    memcpy(rewrite_req+4,requestInfo->url,strlen(requestInfo->url));
    memcpy(rewrite_req+4+strlen(requestInfo->url)," HTTP/1.1\r\n",strlen(" HTTP/1.1\r\n"));
    memcpy(rewrite_req+4+strlen(requestInfo->url)+strlen(" HTTP/1.1\r\n"),"Host: ",6);
    memcpy(rewrite_req+4+strlen(requestInfo->url)+strlen(" HTTP/1.1\r\n")+6,requestInfo->host,strlen(requestInfo->host));
    memcpy(rewrite_req+4+strlen(requestInfo->url)+strlen(" HTTP/1.1\r\n")+6+strlen(requestInfo->host),":",1);
    memcpy(rewrite_req+4+strlen(requestInfo->url)+strlen(" HTTP/1.1\r\n")+6+strlen(requestInfo->host)+1,requestInfo->port,strlen(requestInfo->port));
    memcpy(rewrite_req+4+strlen(requestInfo->url)+strlen(" HTTP/1.1\r\n")+6+strlen(requestInfo->host)+1+strlen(requestInfo->port),"\r\n\r\n",strlen("\r\n\r\n"));
}

int GetConduct(struct RequestInfo *requestInfo, char *request, int sock, struct MyCache* myCache)
{
    struct sockaddr_in serveraddr, serveraddr1; /* server's addr */
    struct sockaddr_in clientaddr;              /* client addr */
    struct hostent *hostp;                      /* client host info */
    char *buf;                                  /* message buffer */
    char *hostaddrp;                            /* dotted decimal host addr string */
    int optval;                                 /* flag value for setsockopt */
    int n;                                      /* message byte size */
    int sockfd;
    struct hostent *server;

    struct ResponseInfo responseInfo;
    char *responseInCache;
    char *p;
    char *temp;
    int age = 0;
    char ageLine[200];
    int j;
    int *responseLength;
    char* rewrite_req;
    char key[ReqBuf];

    buf = (char *)malloc(BUFSIZE);
    temp = (char *)malloc(BUFSIZE);
    responseInCache = (char *)malloc(BUFSIZE);
    responseLength = (int *)malloc(sizeof(int));
    rewrite_req = (char *)malloc(ReqBuf);

    bzero(responseInCache, BUFSIZE);
    bzero(temp, BUFSIZE);
    bzero(responseLength,sizeof(int));
    bzero(key,ReqBuf);

    MakeKey(requestInfo->host, requestInfo->port, requestInfo->url, key);
    int cache_value = getFromMyCache(key, responseInCache, responseLength, myCache);
    if (cache_value == 0)
    {
        age = getAge(key, *myCache);
        sprintf(ageLine, "Age: %d\n", age);
        
        p = strstr(responseInCache, "\r\n");
        p += 2;

        int firstlineLength = p - responseInCache;
        memcpy(temp, responseInCache, firstlineLength);
        // insert age line
        strcat(temp, ageLine);
        memcpy(temp + firstlineLength + strlen(ageLine), p, *responseLength - firstlineLength);
        bzero(buf, BUFSIZE);
        memcpy(buf, temp, *responseLength + strlen(ageLine));
        printf("\nThis is from cache\n");

        n = write(sock, buf, *responseLength + strlen(ageLine));
        if (n <= 0){
            printf("error writing to client from cache!\n");
            return -1;
        }
    }
    else
    {
        // forward to another server
        /* socket: create the socket to server*/
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");

        /* gethostbyname: get the server's DNS entry */
        server = gethostbyname(requestInfo->host);
        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host as %s\n", requestInfo->host);
            return -1;
        }

        /* build the server's Internet address */
        bzero((char *)&serveraddr1, sizeof(serveraddr1));
        serveraddr1.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
              (char *)&serveraddr1.sin_addr.s_addr, server->h_length);
        serveraddr1.sin_port = htons(atoi(requestInfo->port));

        /* connect: create a connection with the server */
        if (connect(sockfd, &serveraddr1, sizeof(serveraddr1)) < 0)
        {   printf("ERROR connecting\n");
            return -1; 
        }
        
        bzero(rewrite_req,ReqBuf);
        rewrite_header(requestInfo, rewrite_req);
        //printf("rewritten request: %s\n",rewrite_req);
        
        // forward to server
        n = write(sockfd, rewrite_req, strlen(rewrite_req));
        if (n <= 0){
            printf("error writing to server\n");
            close(sockfd);
            return sockfd;
        }
        printf("proxy sent %d bytes to server.\n", n);

        // receive server msg
        bzero(buf, BUFSIZE);
        n = read(sockfd, buf, BUFSIZE);
        if (n <= 0){
            close(sockfd);
            return sockfd;
        }

        // get cache info from response
        responseInfo = AnalyzeResponse(buf);
        j = n;
        while (j < responseInfo.contentLength+responseInfo.headerLength)
        {
            n = read(sockfd, buf + j, BUFSIZE-j);
            if (n <= 0)
                break;
            j += n;
        }
        if (j <= 0){
            printf("error reading from server\n");
            close(sockfd);
            return sockfd;
        }

        printf("proxy read %d bytes from server.\n", j);

        // make key as host:port_website
        MakeKey(requestInfo->host, requestInfo->port, requestInfo->url, key);
        
        if(j>=responseInfo.contentLength){
            if (!responseInfo.hasCacheControl)
            {   
                // set default cache
                putIntoMyCache(key, buf, DefaultMaxAge, j, myCache);
            }
            else if (responseInfo.needCache)
            {
                putIntoMyCache(key, buf, responseInfo.maxAge, j, myCache);
            }
        }
        close(sockfd);
        printf("server socket closed\n");

        printf("\nThis is from server (not from cache)\n");

        n = write(sock, buf, j);
        if (n <= 0){
            printf("error writing to client\n");
            return sockfd;
        }
    }
    free(buf);
    free(temp);
    free(responseInCache);
    free(responseLength);
    free(rewrite_req);
    buf = NULL;
    temp = NULL;
    responseInCache = NULL;
    responseLength = NULL;
    rewrite_req = NULL;
    return sockfd; // successfully processed a get request
}