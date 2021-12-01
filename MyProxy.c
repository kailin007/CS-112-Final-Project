/* 
 * proxy.c - A proxy server, support GET and CONNET HTTP method 
 * usage: proxy <port>
 */
#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <resolv.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "text_analysis.h"
#include "my_cache.h"
#include "Client_List.h"
#include "Get_request.h"
#include "Connect_request.h"
#include "SSL_Client.h"

#define MESSIZE 10485760
#define BUFSIZE 2048
#define CacheSize 100
#define DefaultMaxAge 3600
#define ReadBits 2048
#define ClientCapacity 1000

void LoadCertificates(SSL_CTX* ctx, char* KeyFile, char* CertFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

SSL_CTX *InitServerCTX(void)
{
    SSL_CTX *ctx;    
    ctx = SSL_CTX_new(TLSv1_2_server_method());        /* Create new context */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

SSL_CTX *InitClientCTX(void)
{
    SSL_CTX *ctx;
    ctx = SSL_CTX_new(TLSv1_2_client_method());        /* Create new context */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

int main(int argc, char **argv)
{
    int master_socket;             /* listening socket */
    int client_socket;             /* connection socket */
    int portno;                    /* port to listen on */
    int clientlen;                 /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp;         /* client host info */
    char buf[BUFSIZE];             /* message buffer */
    char *hostaddrp;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    int n;                         /* message byte size */
    int sock;
    SSL_CTX *ServerCTX;
    SSL_CTX *ClientCTX;
    SSL *clientssl;
    SSL *serverssl;
    fd_set temp_set, master_set;
    char backup[BUFSIZE + 1];
    struct MyCache myCache;
    struct RequestInfo requestInfo; /* store break down request info */

    SSL_library_init();
    OpenSSL_add_all_algorithms();     /* Load cryptos, et.al. */
    SSL_load_error_strings();         /* Bring in and register error messages */
    ServerCTX = InitServerCTX();
    ClientCTX = InitClientCTX();
    LoadCertificates(ServerCTX, "key.pem", "ca.pem");
    myCache = initializeMyCache(CacheSize, 200, MESSIZE);

    //init my client list
    struct MY_CLIENT **my_client_log = malloc(ClientCapacity * sizeof(struct MY_CLIENT *));
    struct MY_CLIENT ***my_client_p = &my_client_log;

    struct SSL_Client **ssl_log = malloc(ClientCapacity * sizeof(struct SSL_Client *));
    struct SSL_Client ***ssl_p = &ssl_log;



    /* check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /* socket: create a socket */
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
    optval = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    /* build the server's internet address */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;                     /* we are using the Internet */
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);      /* accept reqs to any IP addr */
    serveraddr.sin_port = htons((unsigned short)portno); /* port to listen on */

    /* bind: associate the listening socket with a port */
    if (bind(master_socket, (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    /* listen: make it a listening socket ready to accept connection requests */
    if (listen(master_socket, 5) < 0) /* allow 5 requests to queue up */
        error("ERROR on listen");

    clientlen = sizeof(clientaddr);
    FD_ZERO(&master_set);
    FD_ZERO(&temp_set);

    FD_SET(master_socket, &master_set);
    int fdmax = master_socket;

    int ClientNum = 0; /*current number of client we have in our list*/
    int sslNum = 0; /*current number of server we have in our list*/
    while (1)
    {
        temp_set = master_set;
        select(fdmax + 1, &temp_set, NULL, NULL, NULL);
        if (FD_ISSET(master_socket, &temp_set))
        {
            //make connection to the new client
            client_socket = accept(master_socket, (struct sockaddr *)&clientaddr, &clientlen);
            if (client_socket < 0){
                printf("ERROR on accept\n");
                continue;
            }
            /* gethostbyaddr: determine who sent the message */
            hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                                  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
            if (hostp == NULL){
                printf("ERROR on gethostbyaddr\n");
                close(client_socket);
                continue;
            }
            hostaddrp = inet_ntoa(clientaddr.sin_addr);
            if (hostaddrp == NULL){
                printf("ERROR on inet_ntoa\n");
                close(client_socket);
                continue;
            }
            printf("server established connection with %s (%s) that is socket %d\n",
                   hostp->h_name, hostaddrp, client_socket);

            //put socket in to master set
            FD_SET(client_socket, &master_set);
            if (client_socket > fdmax)
            {
                fdmax = client_socket;
            } 
                            
            if (ClientNum >= ClientCapacity)
            {
                printf("Reach the maximum client capacity, evict some clients!!!!!!!\n");
                int rm_sock = RemoveWhenFull(ClientNum, my_client_p, my_client_log);
                FD_CLR(rm_sock, &master_set);
                close(rm_sock);
                ClientNum =  ClientNum - 1;
            }
            ClientNum = initClient(client_socket, ClientNum, my_client_log);
        }
        else
        {
            //got an message from the client.
            for (sock = 0; sock < fdmax + 1; sock++)
            {
                if (FD_ISSET(sock, &temp_set))
                {
                    // printf("========================================================================\n");
                    // printf("recived a message form client %d\n", sock);
                    // printf("Current clientNum: %d\n",ClientNum);
                    int statusCode = getCode(sock, ClientNum, my_client_p);
                    
                    if (statusCode > 0) //If this sock is either Server or Client with connect method.
                    {
                        int index = FindSSLClient(sock, sslNum, ssl_p);
                        int buff[4000];
                        int m;
                        m = SSL_read((*ssl_p)[index]->sslForThis, buff, 4000);
                        //if the client close the connection, we do the same
                        if (n <= 0)
                        {
                            if (FindClient(sock, ClientNum, my_client_p) >= 0)
                            {
                                //remove this client from list
                                RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                ClientNum = ClientNum - 1;
                                FD_CLR(sock, &master_set);
                                close(sock);
                            }
                            continue;
                        }
                        m = SSL_write((*ssl_p)[index]->sslForOther, buff, m);
                        continue;
                    }
                    else if(statusCode==0 || statusCode==-1 || statusCode==-2) //if this client has status code of 0 or -1 or -2
                    {
                        n = read(sock, buf, BUFSIZE);
                        //if the client close the connection, we do the same
                        if (n <= 0)
                        {
                            //remove this client from list
                            RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                            ClientNum =  ClientNum - 1;
                            FD_CLR(sock, &master_set);
                            close(sock);
                            continue;
                        }
                    }
                    else{ //if this client has status code of -3 (cannot find this client from the list).     
                        write(sock,"Bad Request!",strlen("Bad Request!"));       
                        if(FindClient(sock, ClientNum, my_client_p)>=0){
                                //remove this client from list
                                RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                ClientNum =  ClientNum - 1;
                        }
                        FD_CLR(sock, &master_set);
                        close(sock);
                        continue;
                    }
                    //check if we get a full header.
                    int stas_code;
                    memcpy(backup, buf, n); //make a copy of buf we can so operation
                    backup[n] = '\0';
                    if (strstr(backup, "\r\n\r\n") != NULL)
                    {
                        stas_code = 0;
                    }
                    else
                    {
                        stas_code = -1;
                    }

                    if (stas_code == -1) //we did not get a full request
                    { 
                        //int value = UpdateClient(sock, stas_code, buf, n, ClientNum, my_client_p, my_client_log); //v1: update client message and then keep waiting    
                        write(sock,"Bad Request!",strlen("Bad Request!"));
                        RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                        ClientNum =  ClientNum - 1;
                        FD_CLR(sock, &master_set);
                        close(sock);
                        continue;
                    }
                    else
                    {
                        //complete the request message in client struct and change stas_code to 0.
                        int value = UpdateClient(sock, stas_code, buf, n, ClientNum, my_client_p, my_client_log);
                        int target = FindClient(sock, ClientNum, my_client_p);
                        //printf("%s-------------------------------------------\n", (*my_client_p)[FindClient(sock, ClientNum, my_client_p)]->message);
                        printf("-------------------------------------------\n");
                        char message[500];
                        bzero(message,500);

                        strcpy(message, (*my_client_p)[FindClient(sock, ClientNum, my_client_p)]->message);
                        requestInfo = AnalyzeRequest(message);
                        
                        if (requestInfo.type == 1)
                        {
                            int serverSocket = GetConduct(&requestInfo, message, sock, &myCache);
                            if(serverSocket==-1){write(sock,"Bad Request!",strlen("Bad Request!"));}
                            //remove this client from list
                            RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                            ClientNum =  ClientNum - 1;
                            FD_CLR(sock, &master_set);
                            close(sock);

                            if(FindClient(serverSocket, ClientNum, my_client_p)>=0){
                                RemoveClient(serverSocket, ClientNum, my_client_p, my_client_log);
                                ClientNum =  ClientNum - 1;
                                FD_CLR(serverSocket, &master_set);
                            }
                            continue;
                        }
                        else if (requestInfo.type == 2)
                        {
                            int ServerSocket = ConnectConduct(&requestInfo, sock);


                            // get errors when making connection to server
                            if(ServerSocket==-1){
                                //remove this client from list
                                RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                ClientNum =  ClientNum - 1;
                                FD_CLR(sock, &master_set);
                                close(sock);
                                continue;
                            }

                            FD_SET(ServerSocket, &master_set);
                            if (ServerSocket > fdmax)
                            {
                                fdmax = ServerSocket;
                            }

                            if (ClientNum >= ClientCapacity)
                            {
                                printf("Reach the maximum client capacity, evict some clients!!!!!!!!!!!\n");
                                int rm_sock = RemoveWhenFull(ClientNum, my_client_p, my_client_log);
                                FD_CLR(rm_sock, &master_set);
                                close(rm_sock);
                                ClientNum =  ClientNum - 1;
                            }
                            ClientNum = initClient(ServerSocket, ClientNum, my_client_log);
                            UpdateClient(ServerSocket, sock, "", 0, ClientNum, my_client_p, my_client_log);
                            UpdateClient(sock, ServerSocket, "", 0, ClientNum, my_client_p, my_client_log);


                            //build ssl to client
                            clientssl = SSL_new(ServerCTX);
                            SSL_set_fd(clientssl, sock);
                            int i;
                            if ((i = SSL_accept(clientssl)) <= 0) /* do SSL-protocol accept */
                            {
                                ERR_print_errors_fp(stderr);
                            }
                            //build ssl to server
                            serverssl = SSL_new(ClientCTX);
                            SSL_set_fd(serverssl, ServerSocket);
                            SSL_set_tlsext_host_name(serverssl, requestInfo.host);
                            if ((i = SSL_connect(serverssl)) <= 0) /* do SSL-protocol accept */
                            {
                                ERR_print_errors_fp(stderr);
                            }
                            sslNum = initSSLClient(sock, sslNum, clientssl, serverssl, ssl_log);
                            sslNum = initSSLClient(ServerSocket, sslNum, serverssl, clientssl, ssl_log);
                            continue;
                        }
                        else
                        {
                            printf("Unknow type of HTTP method!\n");
                            write(sock,"Bad Request!",strlen("Bad Request!"));
                            RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                            ClientNum =  ClientNum - 1;
                            FD_CLR(sock, &master_set);
                            close(sock);
                            continue;
                        }
                    }
                }
            }
        }
    }
}
