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

int ForwardHeader(int srcSock, int dstSock)
{
    int n;
    unsigned char buf[BUFSIZ];
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
int ForwardMsg(int srcSock, int dstSock, int length){
    int BUFSIZE = 8192;
    int n;
    char buf[BUFSIZE];
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