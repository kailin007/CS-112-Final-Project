#ifndef MY_CLIENT_ /* Include guard */
#define MY_CLIENT_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BufSize 3000

struct MY_CLIENT
{
    int sock;
    char message[BufSize];
    int status;         //-3 = error, cannot find client in the clientlist; -2 = has not sent request yet; >=0 = this client send a connect method
    int currentLength;
    int bandWidth;
    time_t time;
    int bandWidthLeft;
};
//get the status code of the client using this socket
int getCode(int socket, int Client_Num, struct MY_CLIENT ***myclient_p);
// add first time client into list, return updated number of client
int initClient(int socket, int Client_Num, struct MY_CLIENT **myclient_log, int bandWid);
// update config of this client
int UpdateClient(int socket, int statusCode, char *header, int length, int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log);    
//return index of this client, -1 if such client is not in list
int FindClient(int socket, int Client_Num, struct MY_CLIENT ***myclient_p);
// return updated number of client
int RemoveClient(int socket, int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log);                      
// return updated number of client
int RemoveWhenFull(int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log);                                

#endif // MY_CLIENT_