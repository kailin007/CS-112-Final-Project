#include "SSL_Client.h"

int initSSLClient(int socket, int Client_Num, SSL *This, SSL *Other, struct SSL_Client **myclient_log)
{
    myclient_log[Client_Num] = malloc(sizeof(struct SSL_Client));
    myclient_log[Client_Num] -> sock = socket;
    myclient_log[Client_Num] -> sslForThis = This;
    myclient_log[Client_Num] -> sslForOther = Other;
    Client_Num += 1;
    return Client_Num;
}

int FindSSLClient(int socket, int Client_Num, struct SSL_Client ***myclient_p)
{
    for (int i = 0; i < Client_Num; i++)
    {   
        if ((*myclient_p)[i] -> sock == socket)
        {
            return i;
        }
    }
    return -1;
}

int RemoveSSLClient(int socket, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log){
    int target = FindClient(socket, Client_Num, myclient_p);
    
    if (target == -1)
    {
        printf("Can not find socket you want to remove in the list!\n");
        return -3;
    }
    printf("Removing client %d from our list of client.\n",socket);
    for (int i = target; i < Client_Num-1; i++)
    {
        myclient_log[i] -> sock = (*myclient_p)[i+1] -> sock;
        myclient_log[i] -> sslForOther = (*myclient_p)[i+1] -> sslForOther;
        myclient_log[i] -> sslForThis = (*myclient_p)[i+1] -> sslForThis;
    }
    //free(myclient_log[Client_Num-1]);
    return socket;
}

