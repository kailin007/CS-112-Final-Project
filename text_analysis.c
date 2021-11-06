#include "text_analysis.h"

// parameter: a single line of input file, return the SingleLineInfo struct
struct RequestInfo AnalyzeRequest (char *text){
    struct RequestInfo requestInfo = {-1, "", "", "80"};
    char *buffer, *p1, *p2, *p3;
    
    // GET case
    buffer = strstr(text, "GET");
    if(buffer != NULL){
        p1 = strchr(buffer, ' ');
        p2 = strchr(p1 + 1, ' ');
        // error case
        if(p1 == NULL || p2 == NULL){
            return requestInfo;
        }
        int urlLen = p2 - p1 - 1;
        memcpy(requestInfo.url, p1 + 1, urlLen);
        requestInfo.url[urlLen] = '\0';
        printf("requestInfo.url: %s\n", requestInfo.url);

        // extract host
        buffer = strstr(text, "Host:");
        // error case
        if(buffer == NULL){
            return requestInfo;
        }
        else{
            p1 = strchr(buffer,' ');
            p2 = strchr(p1 + 1,':');
            p3 = strchr(p1 + 1,'\r');

            // error case
            if(p1 == NULL || p3 == NULL){
                return requestInfo;
            }

            if(p2){
                memcpy(requestInfo.host, p1 + 1, p2  - p1 - 1);
                memcpy(requestInfo.port, p2 + 1, p3 - p2 - 1);
            }
            else{
                memcpy(requestInfo.host, p1 + 1, p3  - p1 - 1);
            }
            requestInfo.type = 1;
            printf("requestInfo.host: %s, requestInfo.port: %s\n", requestInfo.host, requestInfo.port);
            return requestInfo;
        }
    }

    // CONNECT case
    buffer = strstr(text, "CONNECT");
    if(buffer != NULL){
        p1 = strchr(buffer,' ');
        p2 = strchr(p1 + 1,':');
        p3 = strchr(p1 + 1,' ');

        // error case
        if(p1 == NULL || p3 == NULL){
            return requestInfo;
        }

        if(p2){
            memcpy(requestInfo.host, p1 + 1, p2  - p1 - 1);
            memcpy(requestInfo.port, p2 + 1, p3 - p2 - 1);
        }
        else{
            memcpy(requestInfo.host, p1 + 1, p3  - p1 - 1);
        }
        requestInfo.type = 2;
        printf("requestInfo.host: %s, requestInfo.port: %s\n", requestInfo.host, requestInfo.port);
        return requestInfo;
    }
    

    return requestInfo;
}


struct ResponseInfo AnalyzeResponse (char* text){
    struct ResponseInfo responseInfo = {0, 0, 0};
    char *buffer, *p1, *p2;
    int i = 0,j = 0;

    buffer = text;
    if((p1 = strstr(buffer, "\r\n\r\n")) != NULL){
        responseInfo.headerLength = p1 - buffer + 4; // + \r\n\r\n
        printf("headerLength: %d\n", responseInfo.headerLength);
    }

    if((p1 = strstr(buffer, "Cache-Control")) != NULL){
        // printf( "Has Cache-Control\n");
        responseInfo.hasCacheControl = 1;
        if((p1 = strstr(buffer, "max-age")) != NULL){
            // printf( "Has max-age\n");
            responseInfo.needCache = 1;
            p1 = strchr(p1, '=');
            p2 = strchr(p1, '\r');
            char maxageStr[10];
            bzero(maxageStr, 10);
            memcpy(maxageStr, p1 + 1, p2 - p1 - 1);
            maxageStr[p2 - p1 - 1] = '\0';
            responseInfo.maxAge = atoi(maxageStr);
            printf("responseInfo.maxAge: %d\n", responseInfo.maxAge);
        }
    }

    if((p1 = strstr(buffer, "Content-Length")) != NULL){
        p1 = strchr(p1, ' ');
        p2 = strchr(p1, '\r');
        char contentLengthStr[10];
        bzero(contentLengthStr, 10);
        memcpy(contentLengthStr, p1 + 1, p2 - p1 - 1);
        contentLengthStr[p2 - p1 - 1] = '\0';
        responseInfo.contentLength = atoi(contentLengthStr);
        printf("Content-Length: %d\n", responseInfo.contentLength);
    }

    return responseInfo;
}
