#ifndef MY_CLIENT_ /* Include guard */
#define MY_CLIENT_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


struct MY_CLIENT
{
    int sock;
    char message[500];
    int status;         //status code 0 = waiting for next request method; -1 = waiting for rest of current request (v2: cause it is not full request, remove the client); 2 = this client send a connect method
    int currentLength;
};
//get the status code of the client using this socket
int getCode(int socket, int Client_Num, struct MY_CLIENT ***myclient_p);
// add first time client into list, return updated number of client
int initClient(int socket, int Client_Num, struct MY_CLIENT **myclient_log);
// update config of this client
int UpdateClient(int socket, int statusCode, char *header, int length, int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log);    
//return index of this client, -1 if such client is not in list
int FindClient(int socket, int Client_Num, struct MY_CLIENT ***myclient_p);
// return updated number of client
int RemoveClient(int socket, int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log);                      
// return updated number of client
int RemoveWhenFull(int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log);                                

#endif // MY_CLIENT_