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

#define CacheSize 100
#define CacheKeySize 3000
#define BUFSIZE 3000  // request buffer/header front 5 bytes
#define MESSIZE 5242880 // cache object size = 5 MB
#define MsgBufSize 30000  // forward Msg size 
#define DefaultMaxAge 600
#define ClientCapacity 500
#define sslCapacity 500
#define blocklistSize 1000

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

int Remove_Client_List_when_full(int ClientNum, struct MY_CLIENT **my_client_log, struct MY_CLIENT ***my_client_p, fd_set* master_set){
    printf("Reach the maximum client capacity, evict some clients!!!!!!!\n");
    int rm_sock = RemoveWhenFull(ClientNum, my_client_p, my_client_log);
    free(my_client_log[ClientNum-1]);
    FD_CLR(rm_sock, master_set);
    close(rm_sock);
    ClientNum =  ClientNum - 1;
    return ClientNum;
}

int Remove_SSL_when_full(int sslNum, struct SSL_Client **ssl_log, struct SSL_Client ***ssl_p, SSL_Return* ssl_return, fd_set* master_set){
    printf("Reach the maximum SSL connections capacity, evict some ssl clients!!!!!!!\n");
    RemoveSSLClientWhenFull(sslNum, ssl_p, ssl_log, ssl_return);
    free(ssl_log[sslNum-1]);
    FD_CLR(ssl_return->sock, master_set);
    SSL_free(ssl_return->sslcon);
    sslNum =  sslNum - 1;
    return sslNum;
}

int Remove_SSL_client(int sock, int sslNum, 
                      fd_set* master_set, struct SSL_Client **ssl_log, struct SSL_Client ***ssl_p, SSL_Return* ssl_return){
        RemoveSSLClient(sock, sslNum, ssl_p, ssl_log, ssl_return);
        free(ssl_log[sslNum-1]);
        FD_CLR(ssl_return->sock, master_set);
        SSL_free(ssl_return->sslcon);
        sslNum =  sslNum - 1;
        return sslNum;
}

int Remove_client(int sock, int ClientNum, struct MY_CLIENT **my_client_log, struct MY_CLIENT ***my_client_p, fd_set* master_set){
        int rm_sock = RemoveClient(sock, ClientNum, my_client_p, my_client_log);
        free(my_client_log[ClientNum-1]);
        FD_CLR(rm_sock, master_set);
        close(rm_sock);
        ClientNum =  ClientNum - 1;
        return ClientNum;
}

int check_hostname(char* blocklists, char* req_host){
   char* temp = (char*)malloc(blocklistSize);
   bzero(temp,blocklistSize);
   memcpy(temp,blocklists,strlen(blocklists));
   char * token = strtok(temp, ",");
   while( token != NULL ) {
      if(strstr(req_host,token)!=NULL){free(temp); return 1;}
      token = strtok(NULL, ",");
   }
   free(temp);
   return 0;
}

int main(int argc, char **argv)
{
    /* check command line args */
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <port> <1(enable trusted proxy)/0(disable trusted proxy)> <blocklists of hostnames(separated by comma;\"NA\" for not blocking)>\n", argv[0]);
        exit(1);
    }
    int portno = atoi(argv[1]);  /* port to listen on */
    int type = atoi(argv[2]); //1(enable trusted proxy),0(disable trusted proxy)
    char* blocklists = argv[3];

    int master_socket;             /* listening socket */
    int client_socket;             /* connection socket */
    int clientlen;                 /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp;         /* client host info */
    char buf[BUFSIZE];
    char Msgbuf[MsgBufSize];
    char message[BUFSIZE];
    char *hostaddrp;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    int n;                         /* message byte size */

    SSL_CTX *ServerCTX;
    SSL_CTX *ClientCTX;
    SSL *clientssl;
    SSL *serverssl;
    struct SSL_Client **ssl_log = malloc(ClientCapacity * sizeof(struct SSL_Client));;
    struct SSL_Client ***ssl_p = &ssl_log;

    struct MY_CLIENT **my_client_log = malloc(ClientCapacity * sizeof(struct MY_CLIENT));
    struct MY_CLIENT ***my_client_p = &my_client_log;

    SSL_Return* ssl_return = malloc(sizeof(SSL_Return));

    fd_set temp_set, master_set;
    char backup[BUFSIZE + 1];
    struct MyCache myCache;
    struct RequestInfo requestInfo; /* store break down request info */
    myCache = initializeMyCache(CacheSize, CacheKeySize, MESSIZE);

    if(type == 1){
        SSL_library_init();
        OpenSSL_add_all_algorithms();     /* Load cryptos, et.al. */
        SSL_load_error_strings();         /* Bring in and register error messages */
        ServerCTX = InitServerCTX();
        ClientCTX = InitClientCTX();
        LoadCertificates(ServerCTX, "key.pem", "ca.pem");
    }

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

    int ClientNum = 0; /*current number of clients we have in our list*/
    int sslNum = 0;
    while (1)
    {
        temp_set = master_set;
        select(fdmax + 1, &temp_set, NULL, NULL, NULL);
        if (FD_ISSET(master_socket, &temp_set))
        {
            /* build tcp connection to the new client */
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

            //put socket into master set
            FD_SET(client_socket, &master_set);
            if (client_socket > fdmax)
            {
                fdmax = client_socket;
            } 
                            
            if (ClientNum >= ClientCapacity)
            {
                ClientNum = Remove_Client_List_when_full(ClientNum, my_client_log, my_client_p, &master_set);
            }

            ClientNum = initClient(client_socket, ClientNum, my_client_log);
        }
        else
        {
            //got an message from the client/server.
            for (int sock = 0; sock < fdmax + 1; sock++)
            {
                if (FD_ISSET(sock, &temp_set))
                {
                    printf("========================================================================\n");
                    printf("Recived a message from socket %d\n", sock);
                    printf("Current clients in total: %d\n",ClientNum);
                    if(type==1){printf("Current ssl clients in total: %d\n",sslNum);}
                    int statuscode = getCode(sock, ClientNum, my_client_p);
 
                    if (statuscode >= 0) //If this sock is either Server or Client with CONNECT method.
                    {
                        if(type == 1){// if this is a trusted proxy
                            int target_sock = getSSLCode(sock, sslNum, ssl_p);
                            if(target_sock<0){
                                sslNum = Remove_SSL_client(sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);
                                ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                                ClientNum = Remove_client(statuscode, ClientNum, my_client_log, my_client_p, &master_set);
                                continue;
                            }

                            bzero(Msgbuf,MsgBufSize);
                            int n = ForwardSSLMsg(sock, target_sock, MsgBufSize, sslNum, ssl_p, ssl_log, Msgbuf, &myCache); 
                            printf("(SSL) Proxy read %d bytes from socket %d and write them to socket %d\n", n, sock, target_sock);
                            
                            if(n<=0){
                                // remove ssl src socket
                                sslNum = Remove_SSL_client(sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);
                                ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                                // remove ssl target socket
                                sslNum = Remove_SSL_client(target_sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);
                                ClientNum = Remove_client(statuscode, ClientNum, my_client_log, my_client_p, &master_set);
                                continue;
                            }
                            continue;
                        }
                        else{// if this is a normal proxy
                            bzero(buf, BUFSIZE);
                            n = read(sock, buf, 1);
                            //if the client close the connection/read call error
                            if (n <= 0)
                            {   
                                ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                                ClientNum = Remove_client(statuscode, ClientNum, my_client_log, my_client_p, &master_set);
                                continue;
                            }

                            int length = MForwardHeader(sock, statuscode, buf); 
                            printf("Message Length = %d\n",length);
                            int needToSent = length;
                            while (needToSent > 0)
                            {
                                if (needToSent <= MsgBufSize)
                                {   
                                    bzero(Msgbuf,MsgBufSize);
                                    int msgBytes = ForwardMsg(sock, statuscode, needToSent, Msgbuf);
                                    if (msgBytes <= 0)
                                    {
                                        ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                                        ClientNum = Remove_client(statuscode, ClientNum, my_client_log, my_client_p, &master_set);
                                        break;
                                    }
                                    needToSent -= msgBytes;
                                }
                                else
                                {   
                                    bzero(Msgbuf,MsgBufSize);
                                    int msgBytes = ForwardMsg(sock, statuscode, MsgBufSize, Msgbuf);
                                    if (msgBytes <= 0)
                                    {
                                        ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                                        ClientNum = Remove_client(statuscode, ClientNum, my_client_log, my_client_p, &master_set);
                                        break;
                                    }
                                    needToSent -= msgBytes;
                                }
                            }
                            continue;
                        }
                    }
                    else if(statuscode==-2) //if this client is sending a new request
                    {
                        bzero(buf,BUFSIZE);
                        n = read(sock, buf, BUFSIZE);
                        if (n <= 0)
                        {   
                            int ssl_tar = FindSSLClient(sock, sslNum, ssl_p);
                            int client_tar = FindClient(sock,ClientNum,my_client_p);
                            if(type == 1 && ssl_tar>=0 && client_tar>=0){
                                int target_sock = getSSLCode(sock, sslNum, ssl_p);
                                // remove ssl src socket
                                sslNum = Remove_SSL_client(sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);
                                ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                                // remove ssl target socket
                                if(target_sock>=0)sslNum = Remove_SSL_client(target_sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);             
                            }
                            else if(client_tar>=0){ 
                                ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                            }
                            continue;
                        }
                    }
                    else{ //if this client is not in the client list     
                          FD_CLR(sock, &master_set);
                          close(sock);
                          continue;
                    }

                    //check if we get a full header.
                    memcpy(backup, buf, n); //make a copy of buf we can so operation
                    backup[n] = '\0';
                    if (strstr(backup, "\r\n\r\n") == NULL) //client sent a bad request
                    {       
                        int ssl_tar = FindSSLClient(sock, sslNum, ssl_p);
                        int client_tar = FindClient(sock,ClientNum,my_client_p);
                        if(type == 1 && ssl_tar>=0 && client_tar>=0){
                            SSL_write(ssl_log[ssl_tar]->sslcon,"Bad Request!",strlen("Bad Request!"));
                            int target_sock = getSSLCode(sock, sslNum, ssl_p);
                            // remove ssl src socket
                            sslNum = Remove_SSL_client(sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);
                            ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                            // remove ssl target socket
                            if(target_sock>=0)sslNum = Remove_SSL_client(target_sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);             
                        }
                        else if(client_tar>=0){ 
                            write(sock,"Bad Request!",strlen("Bad Request!"));
                            ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                        }
                        continue;
                    }

                    UpdateClient(sock, -2, buf, n, ClientNum, my_client_p, my_client_log);
                    printf("-------------------------------------------\n");
                    bzero(message,BUFSIZE);
                    memcpy(message, (*my_client_p)[FindClient(sock, ClientNum, my_client_p)]->message,BUFSIZE);
                    requestInfo = AnalyzeRequest(message);

                    if(strcmp(blocklists,"NA")!=0){ //if there is a blocklist, check if request contains the hostname that should be blocked
                        int re = check_hostname(blocklists, requestInfo.host);
                        if(re==1){
                            int ssl_tar = FindSSLClient(sock, sslNum, ssl_p);
                            int client_tar = FindClient(sock,ClientNum,my_client_p);
                            if(ssl_tar>=0 && client_tar>=0){
                                SSL_write(ssl_log[ssl_tar]->sslcon,"\n\n\r\rThis Request has been blocked!\n\n",strlen("\n\n\r\rThis Request has been blocked!\n\n"));
                                int target_sock = getSSLCode(sock, sslNum, ssl_p);
                                // remove ssl src socket
                                sslNum = Remove_SSL_client(sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);
                                ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                                // remove ssl target socket
                                if(target_sock>=0)sslNum = Remove_SSL_client(target_sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);             
                            }
                            else if(client_tar>=0){ 
                                write(sock,"\n\n\r\rThis Request has been blocked!\n\n",strlen("\n\n\r\rThis Request has been blocked!\n\n"));
                                ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                            }
                            continue;
                        }
                    }
                    
                    if (requestInfo.type == 1) //HTTP Method
                    {
                        int serverSocket = GetConduct(&requestInfo, message, sock, &myCache);
                        if(serverSocket==-1){write(sock,"Bad Request!",strlen("Bad Request!"));}
                        ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                        continue;
                    }
                    else if (requestInfo.type == 2) //HTTPS Method
                    {
                        int ServerSocket = ConnectConduct(&requestInfo, sock);

                        // get errors when making connection to server
                        if(ServerSocket==-1){
                            write(sock,"Bad Request!",strlen("Bad Request!"));
                            ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                            continue;
                        }

                        if(type==1){
                            //build ssl to client
                            clientssl = SSL_new(ServerCTX);     
                            SSL_set_fd(clientssl, sock);
                            int i;
                            if ((i = SSL_accept(clientssl)) <= 0) /* do SSL-protocol accept */
                            {
                                printf("error occurs when doing SSL-protocol accept for client\n");
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

                            if (sslNum >= sslCapacity){
                                sslNum = Remove_SSL_when_full(sslNum, ssl_log, ssl_p, ssl_return, &master_set);
                            }

                            sslNum = initSSLClient(ServerSocket, sslNum, serverssl, ssl_log);
                            sslNum = initSSLClient(sock, sslNum, clientssl, ssl_log);
                            UpdateSSLClient(ServerSocket, sock, "", 0, sslNum, ssl_p, ssl_log);
                            UpdateSSLClient(sock, ServerSocket, "", 0, sslNum, ssl_p, ssl_log);
                        }

                        FD_SET(ServerSocket, &master_set);
                        if (ServerSocket > fdmax)
                        {
                            fdmax = ServerSocket;
                        }

                        if (ClientNum >= ClientCapacity)
                        {
                            ClientNum = Remove_Client_List_when_full(ClientNum, my_client_log, my_client_p, &master_set);
                        }
                        ClientNum = initClient(ServerSocket, ClientNum, my_client_log);
                        UpdateClient(ServerSocket, sock, "", 0, ClientNum, my_client_p, my_client_log);
                        UpdateClient(sock, ServerSocket, "", 0, ClientNum, my_client_p, my_client_log);
                
                        continue;
                    }
                    else
                    {
                        printf("Unknown type of HTTP method!\n");
                        int ssl_tar = FindSSLClient(sock, sslNum, ssl_p);
                        int client_tar = FindClient(sock,ClientNum,my_client_p);
                        if(type == 1 && ssl_tar>=0 && client_tar>=0){
                            SSL_write(ssl_log[ssl_tar]->sslcon,"Bad Request!",strlen("Bad Request!"));
                            int target_sock = getSSLCode(sock, sslNum, ssl_p);
                            // remove ssl src socket
                            sslNum = Remove_SSL_client(sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);
                            ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                            // remove ssl target socket
                            if(target_sock>=0)sslNum = Remove_SSL_client(target_sock, sslNum, &master_set, ssl_log, ssl_p, ssl_return);             
                        }
                        else if(client_tar>=0){ 
                            write(sock,"Bad Request!",strlen("Bad Request!"));
                            ClientNum = Remove_client(sock, ClientNum, my_client_log, my_client_p, &master_set);
                        }
                        continue;               
                    }
                }
                
            }
        }
    }
}
