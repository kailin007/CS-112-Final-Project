#include "Cachereq_request.h"

int CachereqConduct(struct RequestInfo *requestInfo, int sock, struct MyCache *myCache){
    int valueLength = 0;
    char cachedMsg[CacheValueSize];
    int i = getFromMyCache(requestInfo->url, cachedMsg, &valueLength, myCache);
    if(i == -1){
        // if cache request not found
        write(sock, "NA", strlen("NA"));
        close(sock);
        return -1;
    }
    else{
        int n;
        printf("\nStart sending cache content to other proxy (socket: %d)\n", sock);

        n = write(sock, cachedMsg, valueLength);

        if(n <= 0){
            printf("CONNECT: error writing to socket %d\n", sock);
        }
        printf("Sent %d bytes from cache to other proxy (socket: %d)\n\n", n, sock);
        close(sock);
        return 0;
    }
}