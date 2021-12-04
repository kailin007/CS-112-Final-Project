#include "SSL_Client.h"

int getSSLCode(int socket, int Client_Num, struct SSL_Client ***myclient_p){
    int target = FindClient(socket, Client_Num, myclient_p);
    if(target==-1){
        printf("Can not find ssl client you seek for in the ssl list!\n");
        return -3;
    }
    int c_status = (*myclient_p)[target] -> status;
    return c_status;
}

int getSSLMsg(char* msg, int socket, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log){
    int target = FindClient(socket, Client_Num, myclient_p);
    if(target==-1){
        printf("Can not find ssl client you seek for in the ssl list!\n");
        return -3;
    }
    // char* test = myclient_log[target] -> message;
    // int l = myclient_log[target] -> currentLength;
    // printf("before memcpy(msg, (*myclient_p)[target] -> message, (*myclient_p)[target] -> currentLength);\n");
    memcpy(msg, (*myclient_p)[target] -> message, (*myclient_p)[target] -> currentLength);
    return (*myclient_p)[target] -> currentLength;
}

int initSSLClient(int socket, int Client_Num, SSL *sslcon, struct SSL_Client **myclient_log)
{
    myclient_log[Client_Num] = malloc(sizeof(struct SSL_Client));
    myclient_log[Client_Num] -> sock = socket;
    myclient_log[Client_Num] -> sslcon = sslcon;
    myclient_log[Client_Num] -> currentLength = 0;
    myclient_log[Client_Num] -> status = -2;
    Client_Num += 1;
    return Client_Num;
}

int UpdateSSLClient(int socket, int statusCode, char *header, int length, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log){
    int target = FindClient(socket, Client_Num, myclient_p);
    if (target == -1)
    {
        printf("Can not find socket you want to update in the list!\n");
        return -3;
    }
    // printf("before memcpy(myclient_log[target] -> message + (*myclient_p)[target] -> currentLength, header, length);\n");
    memcpy(myclient_log[target] -> message + (*myclient_p)[target] -> currentLength, header, length);  
    myclient_log[target] -> status = statusCode;
    if(statusCode != -1){
        int i = myclient_log[target] -> currentLength;
        myclient_log[target] -> currentLength += length;        // add current length of message, so we can cat the message using it.
    }
    else{
        myclient_log[target] -> currentLength = 0;              // reset current kength of message if we have a complate request.
    }
    char* test = myclient_log[target] -> message;
    int l = myclient_log[target] -> currentLength;

    return 0;
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

SSL_Return* RemoveSSLClient(int socket, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log, SSL_Return* ssl_return){
    int target = FindClient(socket, Client_Num, myclient_p);
    if (target == -1)
    {
        printf("Can not find ssl client you seek for in the ssl list!\n");
        return NULL;
    }

    ssl_return->sslcon = myclient_log[target] -> sslcon;
    ssl_return->sock = myclient_log[target] -> sock;
    
    printf("Removing ssl client %d from our ssl list of client.\n",socket);
    for (int i = target; i < Client_Num-1; i++)
    {
        myclient_log[i] -> sock = (*myclient_p)[i+1] -> sock;
        myclient_log[i] -> sslcon = (*myclient_p)[i+1] -> sslcon;
        strcpy(myclient_log[i] -> message, (*myclient_p)[i+1] -> message);
        myclient_log[i] -> status = (*myclient_p)[i+1] -> status;
    }

    return ssl_return;
}

//when the list is full, delete client that has finished a request first (status code = 0): if not exist such client, evict the first client in the list
SSL_Return* RemoveSSLClientWhenFull(int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log, SSL_Return* ssl_return){
    for (int i = 0; i < Client_Num; i++)
    {
        if((*myclient_p)[i] -> status == -1)
        {
            return RemoveSSLClient((*myclient_p)[i] -> sock, Client_Num, myclient_p, myclient_log, ssl_return);
        }
        if((*myclient_p)[i] -> status == -2)
        {
            return RemoveSSLClient((*myclient_p)[i] -> sock, Client_Num, myclient_p, myclient_log, ssl_return);
        }
    }
    return RemoveSSLClient((*myclient_p)[0] -> sock, Client_Num, myclient_p, myclient_log, ssl_return);
}