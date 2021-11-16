#include "Client_List.h"


int initClient(int socket, int Client_Num, struct MY_CLIENT **myclient_log){
    myclient_log[Client_Num] = malloc(sizeof(struct MY_CLIENT));
    myclient_log[Client_Num] -> sock = socket;
    myclient_log[Client_Num] -> currentLength = 0;
    myclient_log[Client_Num] -> status = -2;
    Client_Num += 1;
    return Client_Num;
}

int FindClient(int socket, int Client_Num, struct MY_CLIENT ***myclient_p){
    for (int i = 0; i < Client_Num; i++)
    {
        if ((*myclient_p)[i] -> sock == socket)
        {
            return i;
        }
    }
    return -1;
}

int getCode(int socket, int Client_Num, struct MY_CLIENT ***myclient_p){
    int target = FindClient(socket, Client_Num, myclient_p);
    return (*myclient_p)[target] -> status;
}

int UpdateClient(int socket, int statusCode, char *header, int length, int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log){
    int target = FindClient(socket, Client_Num, myclient_p);

    if (target == -1)
    {
        printf("Can not find socket you want to update in the list!\n");
        return -1;
    }

    memcpy(myclient_log[target] -> message + (*myclient_p)[target] -> currentLength, header, length);
    myclient_log[target] -> status = statusCode;
    if(statusCode != -1){
        myclient_log[target] -> currentLength += length;        // add current length of message, so we can cat the message using it.
    }
    else{
        myclient_log[target] -> currentLength = 0;              // reset current kength of message if we have a complate request.
    }
    return 0;
}   

 

int RemoveClient(int socket, int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log){
    int target = FindClient(socket, Client_Num, myclient_p);
    
    if (target < 0)
    {
        printf("Can not find socket you want to remove in the list!\n");
        return -1;
    }
    printf("Removing client %d from our list of client.\n",socket);
    for (int i = target; i < Client_Num-1; i++)
    {
        myclient_log[i] -> sock = (*myclient_p)[i+1] -> sock;
        strcpy(myclient_log[i] -> message, (*myclient_p)[i+1] -> message);
        myclient_log[i] -> status = (*myclient_p)[i+1] -> status;
    }
    free(myclient_log[Client_Num-1]);
    return socket;
}                


//when the list is full delete client has not yet sent a full request first in first out, then client with 
int RemoveWhenFull(int Client_Num, struct MY_CLIENT ***myclient_p, struct MY_CLIENT **myclient_log){
    for (int i = 0; i < Client_Num; i++)
    {
        if((*myclient_p)[i] -> status == 0)
        {
            return RemoveClient((*myclient_p)[i] -> status, Client_Num, myclient_p, myclient_log);
        }
    }
    for (int i = 0; i < Client_Num; i++)
    {
        if((*myclient_p)[i] -> status == -1)
        {
            return RemoveClient((*myclient_p)[i] -> status, Client_Num, myclient_p, myclient_log);
        }
    }
    return RemoveClient((*myclient_p)[0] -> status, Client_Num, myclient_p, myclient_log);
}