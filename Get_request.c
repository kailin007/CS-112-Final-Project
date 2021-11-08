#include "Get_request.h"

#define BUFSIZE 10485760
#define CacheSize 10
#define DefaultMaxAge 3600
#define ReadBits 1024

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}



void MakeKey(char *Host, char *Port, char *GET, char *key)
{
    strcpy(key, "");
    strcpy(key, Host);
    strcat(key, ":");
    strcat(key, Port);
    strcat(key, "_");
    strcat(key, GET);
}


int GetRequest(char *request, int sock, struct MyCache myCache)
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

    struct RequestInfo requestInfo;
    struct ResponseInfo responseInfo;
    char *responseInCache;
    char *p;
    char *temp;
    int age = 0;
    char ageLine[200];
    int j;
    int *responseLength;

    buf = (char *)malloc(BUFSIZE * sizeof(char));
    temp = (char *)malloc(BUFSIZE * sizeof(char));
    responseInCache = (char *)malloc(BUFSIZE * sizeof(char));
    responseLength = (int *)malloc(sizeof(int));
    char key[200];

    requestInfo = AnalyzeRequest(request);

    bzero(responseInCache, BUFSIZE);
    bzero(temp, BUFSIZE);
    // find in cache

    MakeKey(requestInfo.host, requestInfo.port, requestInfo.url, key);
    getFromMyCache(key, responseInCache, responseLength, &myCache);
    printf("key forage succ\n");
    if (strcmp(responseInCache, "NA") != 0)
    {

        age = getAge(key, myCache);
        printf("Age: %d\n", age);
        sprintf(ageLine, "Age: %d\n", age);
        // printf("ageLine: %s\n", ageLine);
        p = strstr(responseInCache, "\r\n");
        p += 2;

        int firstlineLength = p - responseInCache;
        memcpy(temp, responseInCache, firstlineLength);
        // insert age line
        strcat(temp, ageLine);
        memcpy(temp + firstlineLength + strlen(ageLine), p, *responseLength - firstlineLength);
        bzero(buf, BUFSIZE);
        memcpy(buf, temp, *responseLength + strlen(ageLine));
        printf("\nThis is from cache:\n");

        n = write(sock, buf, *responseLength + strlen(ageLine));
    }
    else
    {
        // forward to another server
        /* socket: create the socket to server*/
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");

        /* gethostbyname: get the server's DNS entry */
        server = gethostbyname(requestInfo.host);
        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host as %s\n", requestInfo.host);
            exit(0);
        }

        /* build the server's Internet address */
        bzero((char *)&serveraddr1, sizeof(serveraddr1));
        serveraddr1.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
              (char *)&serveraddr1.sin_addr.s_addr, server->h_length);
        serveraddr1.sin_port = htons(requestInfo.port);

        /* connect: create a connection with the server */
        if (connect(sockfd, &serveraddr1, sizeof(serveraddr1)) < 0)
            error("ERROR connecting");

        // forward to server
        n = write(sockfd, request, strlen(request));
        if (n < 0)
            error("ERROR reading from socket");
        printf("proxy sent %d bytes.\n", n);

        // receive server msg
        bzero(buf, BUFSIZE);
        n = read(sockfd, buf, BUFSIZE);

        // get cache info from response
        responseInfo = AnalyzeResponse(buf);

        j = n;

        while (j < responseInfo.contentLength)
        {
            n = read(sockfd, buf + j, BUFSIZE);
            if (n < 0)
                break;
            j += n;
        }

        // make key as host:port_website
        MakeKey(requestInfo.host, requestInfo.port, requestInfo.url, key);
        if (!responseInfo.hasCacheControl)
        {
            // set default cache
            putIntoMyCache(key, buf, DefaultMaxAge, j, &myCache);
        }
        else if (responseInfo.needCache)
        {
            putIntoMyCache(key, buf, responseInfo.maxAge, j, &myCache);
        }

        printf("proxy received %d bytes.\n", n);
        close(sockfd);
        printf("sockfd closed\n");

        printf("\nThis is from server(not from cache):\n");

        n = write(sock, buf, j);
        printf("****************************************************************************************\n\n");
    }
    free(buf);
    free(responseInCache);
    free(temp);
    free(responseLength);
}