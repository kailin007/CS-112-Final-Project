#ifndef MY_GET_ /* Include guard */
#define MY_GET_

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

int GetConduct(struct RequestInfo *requestInfo, char *request, int sock, struct MyCache* myCache);
void rewrite_header(struct RequestInfo *requestInfo, char* rewrite_req);
                   
#endif // MY_GET_