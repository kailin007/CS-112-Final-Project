/* 
 * proxy.c - A proxy server, support GET and CONNET HTTP method 
 * usage: proxy <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "text_analysis.h"
#include "my_cache.h"
#include "Client_List.h"
#include "Get_request.h"
#include "Connect_request.h"

#define MESSIZE 10485760
#define BUFSIZE 2048
#define CacheSize 100
#define DefaultMaxAge 3600
#define ReadBits 2048
#define ClientCapacity 1000

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
    fd_set temp_set, master_set;
    char backup[BUFSIZE + 1];
    struct MyCache myCache;
    struct RequestInfo requestInfo; /* store break down request info */

    myCache = initializeMyCache(CacheSize, 200, MESSIZE);

    //init my client list
    struct MY_CLIENT **my_client_log = malloc(ClientCapacity * sizeof(struct MY_CLIENT *));
    struct MY_CLIENT ***my_client_p = &my_client_log;

    struct MY_CLIENT **server_log = malloc(ClientCapacity * sizeof(struct MY_CLIENT *));
    struct MY_CLIENT ***server_p = &server_log;

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
    int ServerNum = 0; /*current number of server we have in our list*/
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
                    printf("========================================================================\n");
                    printf("recived a message form client %d\n", sock);
                    printf("Current clientNum: %d\n",ClientNum);
                    int statusCode = getCode(sock, ClientNum, my_client_p);
                    
                    if (statusCode > 0) //If this sock is either Server or Client with connect method.
                    {
                        printf("Find the status code(>0) for socket\n");
                        bzero(buf, BUFSIZE);
                        n = read(sock, buf, 1);

                        //if the client close the connection, we do the same
                        if (n <= 0)
                        {   
                            if(FindClient(sock, ClientNum, my_client_p)>=0){
                                //remove this client from list
                                RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                ClientNum =  ClientNum - 1;
                                FD_CLR(sock, &master_set);
                                close(sock);
                            }
                            continue;
                        }
                        //If this is a application type of tls message, forward to other side.
                        short code = 0;
                        memcpy((char *)&code, buf, 1);
                        code = ntohs(code);
                        if (code <= 5888)
                        {
                            int target_sock = getCode(sock, ClientNum, my_client_p);
                            int length = MForwardHeader(sock, target_sock, buf); 
                            if(length==-1){ // get error when forwarding
                                if(FindClient(sock, ClientNum, my_client_p)>=0){
                                    RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                    close(sock);
                                    ClientNum =  ClientNum - 1;
                                    FD_CLR(sock, &master_set);
                                }
                                if(FindClient(target_sock, ClientNum, my_client_p)>=0){
                                    RemoveClient(target_sock, ClientNum, my_client_p, my_client_log);
                                    close(target_sock);
                                    ClientNum =  ClientNum - 1;
                                    FD_CLR(target_sock, &master_set);
                                }
                                continue; // break current while loop     
                            }
                            printf("Length = %d\n",length);
                            int needToSent = length;
                            while (needToSent != 0)
                            {
                                if (needToSent < 8000)
                                {
                                    if (ForwardMsg(sock, target_sock, needToSent) == -1) // get error when forwarding
                                    {
                                        if(FindClient(sock, ClientNum, my_client_p)>=0){
                                            RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                            close(sock);
                                            ClientNum =  ClientNum - 1;
                                            FD_CLR(sock, &master_set);
                                        }
                                        if(FindClient(target_sock, ClientNum, my_client_p)>=0){
                                            RemoveClient(target_sock, ClientNum, my_client_p, my_client_log);
                                            close(target_sock);
                                            ClientNum =  ClientNum - 1;
                                            FD_CLR(target_sock, &master_set);
                                        }
                                        break; // break current while loop                 
                                    }
                                    needToSent = 0;
                                }
                                else
                                {       
                                    if (ForwardMsg(sock, target_sock, 8000) == -1) // get error when forwarding
                                    {
                                        if(FindClient(sock, ClientNum, my_client_p)>=0){
                                            RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                            close(sock);
                                            ClientNum =  ClientNum - 1;
                                            FD_CLR(sock, &master_set);
                                        }
                                        if(FindClient(target_sock, ClientNum, my_client_p)>=0){
                                            RemoveClient(target_sock, ClientNum, my_client_p, my_client_log);
                                            close(target_sock);
                                            ClientNum =  ClientNum - 1;
                                            FD_CLR(target_sock, &master_set);
                                        }
                                        break; // break current while loop
                                    }
                                    needToSent -= 8000;
                                }
                            }
                            continue;
                        }

                        //If its not, read the rest of the buffer.    
                        n = read(sock, buf+1, BUFSIZE-1);

                        //if the client close the connection, we do the same
                        if (n <= 0)
                        {
                            if(FindClient(sock, ClientNum, my_client_p)>=0){
                                //remove this client from list
                                RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                ClientNum =  ClientNum - 1;
                                FD_CLR(sock, &master_set);
                                close(sock);
                            }
                            continue;
                        }
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
                            //TODO: make tcp connection with the https server
                            //      sent 200 ok back to client
                            //      waiting for the client to sent more 
                            int ServerSocket = ConnectConduct(&requestInfo, sock);
                            // get errors when making connection to server
                            if(ServerSocket==-1){
                                //remove this client from list
                                RemoveClient(sock, ClientNum, my_client_p, my_client_log);
                                ClientNum =  ClientNum - 1;
                                FD_CLR(sock, &master_set);
                                close(sock);
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
