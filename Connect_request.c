#include "Connect_request.h"
#define h_addr h_addr_list[0]

// establish TCP connect to server according to CONNECT request
int ConnectConduct(struct RequestInfo *requestInfo){
    int serverSock;
    struct sockaddr_in serveraddr; /* server's addr */
    struct hostent *server;


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
        return 0;
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
        return 0; 
    }
    
    return 1; // successfully exit
}

// forward messages from srcSock to dstSock; return the length of message.
int ForwardMsg(int srcSock, int dstSock){
    int BUFSIZE = 8192;
    int n;
    char buf[BUFSIZE];

    n = read(srcSock, buf, BUFSIZE);
    if(n<=0){
        printf("error reading\n");
        return n;
    }
    n = write(dstSock, buf, n);
    if(n<=0){
        printf("error writing\n");
        return n;
    }

    return n;
}