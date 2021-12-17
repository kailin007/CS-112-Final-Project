#ifndef CACHEREQ_REQUEST_ /* Include guard */
#define CACHEREQ_REQUEST_

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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "text_analysis.h"
#include "SSL_Client.h"
#include "my_cache.h"

#define CacheMaxAge 600
#define CacheValueSize 5242880
#define MaxUrlLength 3000

int CachereqConduct(struct RequestInfo *requestInfo, int sock, struct MyCache *myCache);

#endif // CACHEREQ_REQUEST_