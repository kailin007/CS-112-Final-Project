#include "Connect_request.h"

#define h_addr h_addr_list[0]

// establish TCP connect to server according to CONNECT request
int ConnectConduct(struct RequestInfo *requestInfo, int sock){
    int serverSock;
    struct sockaddr_in serveraddr; /* server's addr */
    struct hostent *server;
    int n = 0;
    unsigned char buf[BUFSIZ];
    int length;


    // establish TCP connection to server
    /* socket: create the socket to server*/
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(requestInfo->host);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host as %s\n", requestInfo->host);
        return -1;
    }

    /* build the server's Internet address */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
            (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(atoi(requestInfo->port));

    /* connect: create a connection with the server */
    if (connect(serverSock, &serveraddr, sizeof(serveraddr)) < 0)
    {   printf("ERROR connecting\n");
        return -1; 
    }
    //sent 200 ok to client
    n = write(sock, "HTTP/1.1 200 OK\r\n\r\n", strlen("HTTP/1.1 200 OK\r\n\r\n"));
    printf("CONNECT: successfully connect to the server\n");
    return serverSock; // successfully exit
}

int ForwardHeader(int srcSock, int dstSock, unsigned char *buf)
{
    int n;
    short number = 0;
    n = read(srcSock, buf, 5);
    if(n<=0){
        printf("CONNECT: error reading from socket %d\n", srcSock);
        return -1;
    }
    memcpy((char*)&number, buf+3, 2);
    n = write(dstSock, buf, 5);
    if(n<=0){
        printf("CONNECT: error writing to socket %d\n", dstSock);
        return -1;
    }
    number = ntohs(number);
    return number;
}

int MForwardHeader(int srcSock, int dstSock, unsigned char *buf)
{
    int n;
    short number = 0;
    n = read(srcSock, buf+1, 4);
    if(n<=0){
        printf("CONNECT: error reading from socket %d\n", srcSock);
        return -1;
    }
    memcpy((char*)&number, buf+3, 2);
    n = write(dstSock, buf, 5);
    if(n<=0){
        printf("CONNECT: error writing to socket %d\n", dstSock);
        return -1;
    }
    number = ntohs(number);
    return number;
}

// forward messages from srcSock to dstSock; return the length of message.
int ForwardMsg(int srcSock, int dstSock, int length, char* buf){
    int n;
    int recved = 0;
    while (recved < length)
    {
        n = read(srcSock, buf + recved, length - recved);
        recved += n;
    }
    
    if(n<=0){
        printf("CONNECT: error reading from socket %d\n", srcSock);
        return -1;
    }

    n = write(dstSock, buf, length);
    if(n<=0){
        printf("CONNECT: error writing to socket %d\n", dstSock);
        return -1;
    }

    return n;
}

// forward messages from srcSock to dstSock; return the length of message.
int ForwardSSLMsg(int srcSock, int dstSock, int bufSize, int ClientNum, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log, char *buf, struct MyCache* myCache)
{
    int n;
    int srctag = FindSSLClient(srcSock, ClientNum, myclient_p);
    if (srctag == -1)
        return -1;
    int dsttag = FindSSLClient(dstSock, ClientNum, myclient_p);
    if (dsttag == -1)
        return -1;
    SSL *srcSSL = myclient_log[srctag]->sslcon;
    SSL *dstSSL = myclient_log[dsttag]->sslcon;

    struct RequestInfo requestInfo;

    bzero(buf, bufSize);
    n = SSL_read(srcSSL, buf, bufSize);
    if (n <= 0)
    {
        printf("CONNECT: error reading from socket %d\n", srcSock);
    }
    else{
        char msg[MaxUrlLength];
        bzero(msg, MaxUrlLength);
        getSSLMsg(msg, dstSock, ClientNum, myclient_p, myclient_log);
        if(strcmp(msg, "exceed_cache_value_size") != 0){
            // content size doesn't exceed cache value size, cache it
            requestInfo = AnalyzeRequest(buf);
            if(requestInfo.type == 1){
                // if it's request, save the request header to client message
                char host_url[MaxUrlLength];
                MakeKey(requestInfo.host, requestInfo.port, requestInfo.url, host_url);
                int i = strlen(host_url);
                UpdateSSLClient(dstSock, srcSock, host_url, strlen(host_url) + 1, ClientNum, myclient_p, myclient_log);
            }
            else{
                // msg is from the server, save it to cache
                char host_url[MaxUrlLength], *cachedMsg;
                cachedMsg = (char *) malloc (CacheValueSize * sizeof(char));
                int headerLength = 0, valueLength = 0;
                // get request header
                bzero(host_url, MaxUrlLength);
                headerLength = getSSLMsg(host_url, dstSock, ClientNum, myclient_p, myclient_log);
                // make it a string
                host_url[headerLength] = '\0';
                // get already cached part
                int i = getFromMyCache(host_url, cachedMsg, &valueLength, myCache);

                // connect new part
                if(n + valueLength < CacheValueSize){
                    // printf("before memcpy(cachedMsg + valueLength, buf, n);\n");
                    memcpy(cachedMsg + valueLength, buf, n);
                    // save it back to cache
                    putIntoMyCache(host_url, cachedMsg, CacheMaxAge, valueLength + n, myCache);
                }
                else{
                    // if content exceed cache value size, delete it from cache and don't cache it
                    deleteFromMyCache(host_url, myCache);
                    UpdateSSLClient(dstSock, srcSock, "exceed_cache_value_size", strlen("exceed_cache_value_size") + 1, ClientNum, myclient_p, myclient_log);
                }
                if(cachedMsg != NULL){
                    // in case of double free error (minor case)
                    free(cachedMsg);
                }
            }
        } 
    }
    

    n = SSL_write(dstSSL, buf, n);
    if (n <= 0)
    {
        printf("CONNECT: error writing to socket %d\n", dstSock);
    }

    // if received the end of the response, return 0 and close both sockets at main
    if(requestInfo.type != 1 && buf[n-5]=='0' && buf[n-4]=='\r' && buf[n-3]=='\n' && buf[n-2]=='\r' && buf[n-1]=='\n'){
        printf("Cache used: %d\n", numUsed(*myCache));
        return 0;
    }

    return n;
}