#ifndef MY_CONNECT_ /* Include guard */
#define MY_CONNECT_

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

int ConnectConduct(struct RequestInfo *requestInfo); // establish TCP connect to server according to CONNECT request
int ForwardMsg(int srcSock, int dstSock); // forward messages from srcSock to dstSock; return the length of message.

                     

#endif // MY_CONNECT_