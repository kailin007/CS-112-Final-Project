/* 
 * proxy.c - A proxy server, support GET and CONNET HTTP method 
 * usage: proxy <port>
 * 
 * 
 * 
 * 11/6 update: support multiple client,
 *              separate GET method from main file,
 *              add functionality to support CONNECT method
 * 
 * TODO: Error Handling,
 *       Performance Testing
 *       Additonal Functionality...
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

#define MESSIZE 10485760
#define BUFSIZE 1024
#define CacheSize 10
#define DefaultMaxAge 3600
#define ReadBits 1024
#define ClientCapacity 100

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

    while (1)
    {
        temp_set = master_set;
        select(fdmax + 1, &temp_set, NULL, NULL, NULL);
        if (FD_ISSET(master_socket, &temp_set))
        {
            //make connection to the new client
            client_socket = accept(master_socket, (struct sockaddr *)&clientaddr, &clientlen);
            if (client_socket < 0)
                error("ERROR on accept");
            /* gethostbyaddr: determine who sent the message */
            hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                                  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
            if (hostp == NULL)
                error("ERROR on gethostbyaddr");
            hostaddrp = inet_ntoa(clientaddr.sin_addr);
            if (hostaddrp == NULL)
                error("ERROR on inet_ntoa\n");
            printf("server established connection with %s (%s) that is socket %d\n",
                   hostp->h_name, hostaddrp, client_socket);

            //put socket in to master set
            FD_SET(client_socket, &master_set);
            if (client_socket > fdmax)
            {
                fdmax = client_socket;
            }
            //init this first time client socket
            if (ClientNum == ClientCapacity)
                RemoveWhenFull(ClientNum, my_client_p, my_client_log);
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

                    if (getCode(sock, ClientNum, my_client_p) == 1)
                    {
                        //TODO: sent what ever client send us to the server
                        //      and sent server respond to client
                        continue;
                    }

                    bzero(buf, BUFSIZE);
                    n = read(sock, buf, BUFSIZE);
                    //if the client close the connection, we do the same
                    if (n <= 0)
                    {
                        //remove this client from list
                        ClientNum = RemoveClient(sock, ClientNum, my_client_p, my_client_log);
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

                    if (stas_code == -1)
                    { //we did not get a full request
                        UpdateClient(sock, stas_code, buf, n, ClientNum, my_client_p, my_client_log);
                        continue; //update client message and then keep waiting
                    }
                    else
                    {
                        //complate the request message in client struct and change stas_code to 0.
                        UpdateClient(sock, stas_code, buf, n, ClientNum, my_client_p, my_client_log);
                        printf("%s--------------\n", (*my_client_p)[FindClient(sock, ClientNum, my_client_p)]->message);
                        char *message[500];
                        strcpy(message, (*my_client_p)[FindClient(sock, ClientNum, my_client_p)]->message);
                        requestInfo = AnalyzeRequest(message);

                        if (requestInfo.type == 1)
                        {
                            GetConduct(requestInfo, message, sock, myCache);
                            continue;
                        }
                        else if (requestInfo.type == 2)
                        {
                            UpdateClient(sock, stas_code, "", 0, ClientNum, my_client_p, my_client_log);
                            //TODO: make tcp connection with the https server
                            //      sent 200 ok back to client
                            //      waiting for the client to sent more
                            continue;
                        }
                        else
                        {
                            printf("Unknow type of HTTP method!\n");
                            continue;
                        }
                    }
                }
            }
        }
    }
}