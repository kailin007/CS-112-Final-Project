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
    SSL *sslcon;
    char message[1000];
    int status;         //-3 = error, cannot find client in the clientlist; -2 = has not sent request yet; >=0 = this client send a connect method
    int currentLength;
};

typedef struct SSl_Return
{
    int sock;
    SSL *sslcon;
}SSL_Return;

int getSSLCode(int socket, int Client_Num, struct SSL_Client ***myclient_p);
int initSSLClient(int socket, int Client_Num, SSL *sslcon, struct SSL_Client **myclient_log);
int FindSSLClient(int socket, int Client_Num, struct SSL_Client ***myclient_p);
int UpdateSSLClient(int socket, int statusCode, char *header, int length, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log);
SSL_Return* RemoveSSLClient(int socket, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log, SSL_Return* ssl_return);    
SSL_Return* RemoveSSLClientWhenFull(int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log, SSL_Return* ssl_return);    
int getSSLMsg(char* msg, int socket, int Client_Num, struct SSL_Client ***myclient_p, struct SSL_Client **myclient_log);
                                
#endif // SSL_Client_