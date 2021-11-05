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
