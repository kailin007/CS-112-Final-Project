#include "Connect_request.h"

int ConnectConduct(struct RequestInfo *requestInfo, char *request, int clientSock, struct MyCache* myCache){
    int n;
    int j;
    int serverSock;
    struct sockaddr_in serveraddr; /* server's addr */
    struct hostent *server;
    char *buf = (char *)malloc(BUFSIZE * sizeof(char));

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
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
            (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(atoi(requestInfo->port));

    /* connect: create a connection with the server */
    if (connect(serverSock, &serveraddr, sizeof(serveraddr)) < 0)
        error("ERROR connecting");
    
    // send OK message to client
    n = write(clientSock, "HTTP/1.1 200 OK\r\n\r\n", strlen("HTTP/1.1 200 OK\r\n\r\n") + 1);
    printf("proxy sent %d bytes to client.\n", n);

    // forward messages
    while(1){
        bzero(buf, BUFSIZE);
        n = read(clientSock, buf, BUFSIZE);
        if (n <= 0) break;
        printf("proxy received %d bytes from client.\n", n);
        n = write(serverSock, buf, n);
        printf("proxy sent %d bytes to server.\n", n);
        // else {
        //     while(1){
        //         printf("proxy received %d bytes from client.\n", n);
        //         n = write(serverSock, buf, n);
        //         if (n == 0) break;
        //         printf("proxy sent %d bytes to server.\n", n);
        //         bzero(buf, BUFSIZE);
        //         n = read(clientSock, buf, BUFSIZE);
        //         if (n <= 0) break;
        //     }
        // }


        bzero(buf, BUFSIZE);
        n = read(serverSock, buf, BUFSIZE);
        if (n <= 0) break;
        printf("proxy received %d bytes from server.\n", n);
        n = write(clientSock, buf, n);
        printf("proxy sent %d bytes to client.\n", n);
        // else {
        //     while(1){
        //         printf("proxy received %d bytes from server.\n", n);
        //         n = write(clientSock, buf, n);
        //         if (n == 0) break;
        //         printf("proxy sent %d bytes to client.\n", n);
        //         bzero(buf, BUFSIZE);
        //         n = read(serverSock, buf, BUFSIZE);
        //         if (n <= 0) break;
        //     }
        // }
        
        

        
    }
    close(serverSock);
    printf("serverSock closed\n");
    
    free(buf);
}