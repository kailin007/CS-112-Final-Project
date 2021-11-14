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
#include "my_cache.h"
#include "Client_List.h"

#define BUFSIZE 102400

int ConnectConduct(struct RequestInfo *requestInfo);

                     

#endif // MY_CONNECT_