#ifndef CLIENT_ /* Include guard */
#define CLIENT_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <pthread.h>
#include <sys/select.h>

typedef struct socket_list
{
    int socket;
    struct socket_list *next;
} socket_list;

typedef struct __DATA
{
     char *hostname;
     int portno;
     struct hostent *server;
}DATA;

typedef struct __ReqDATA
{
     char *request;
     char *hostname;
     int portno;
     struct hostent *server;
}ReqDATA;

char *one_client_makes_connection_to_proxy(DATA* data);
char *multiple_clients_make_connections_to_proxy(int client_num, char *hostname, int portno, char *test_condition);
char *client_exit(char *request, char *hostname, int portno, char *exist_condition);
char *one_client_sends_request_to_server(ReqDATA* data);
char *multiple_clients_send_requests_to_server(int client_num, char *requests[client_num], char *hostname, int portno, char *send_condition);                          

#endif // CLIENT_