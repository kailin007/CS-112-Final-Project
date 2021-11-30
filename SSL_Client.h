#ifndef SSL_Client_ /* Include guard */
#define SSL_Client_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


struct SSL_Client
{
    int sock;
    SSL *sslForThis;
    SSL *sslForOther;
};
// add first time client into list, return updated number of client
int initSSLClient(int socket, int Client_Num, SSL *This, SSL *Other, struct SSL_Client **myclient_log);
//return index of this client, -1 if such client is not in list
int FindSSLClient(int socket, int Client_Num, struct SSL_Client ***myclient_p);
// return updated number of client
int RemoveSSLClient(int socket, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log);                      
                                

#endif // SSL_Client_