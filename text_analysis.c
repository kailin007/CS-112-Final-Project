#include "text_analysis.h"

void getHostFromUrl(char* url, char* host, char* port){
    char *p1, *p2, *p3, *p4, *start, *end;
    // default port: 80
    strcpy(port, "80");

    p1 = strstr(url, "//");
    if(p1 == NULL){
        // if no prototal
        start = url;
    }
    else{
        start = p1 + 2;
    }

    p2 = strchr(start, '/');
    p3 = strchr(start, '\0');
    p4 = strchr(start, ':');

    if(p2 != NULL){
        end = p2 - 1;
    }
    else{
        end = p3 - 1;
    }

    int hostLength = end - start + 1;
    memcpy(host, start, hostLength);
    host[hostLength] = '\0'; // make it a string

    if(p4 != NULL){
        // if there's port
        int portLength = end - (p4 + 1) + 1;
        memcpy(port, p4 + 1, portLength);
        port[portLength] = '\0';
    }
}

// parameter: a single line of input file, return the SingleLineInfo struct
struct RequestInfo AnalyzeRequest(char *text)
{
    struct RequestInfo requestInfo = {-1, "", "", "80"};
    char *buffer, *p1, *p2, *p3;

    // GET case
    buffer = strstr(text, "GET");
    if (buffer != NULL)
    {
        if(strstr(text, "Host:")==NULL){return requestInfo;} //newly added
        
        p1 = strchr(buffer, ' ');
        p2 = strchr(p1 + 1, ' ');
        // error case
        if (p1 == NULL || p2 == NULL)
        {
            return requestInfo;
        }
        int urlLen = p2 - p1 - 1;
        memcpy(requestInfo.url, p1 + 1, urlLen);
        requestInfo.url[urlLen] = '\0';
        printf("requestInfo.url: %s\n", requestInfo.url);

        // extract host
        buffer = strstr(text, "Host:");
        // error case
        if (buffer == NULL)
        {
            //getHostFromUrl(requestInfo.url, requestInfo.host, requestInfo.port); //has some bugs
            return requestInfo;
        }
        else
        {
            p1 = strchr(buffer, ' ');
            p2 = strchr(p1 + 1, ':');
            p3 = strchr(p1 + 1, '\r');

            // error case
            if (p1 == NULL || p3 == NULL)
            {
                return requestInfo;
            }

            if (p2 != NULL && p2 < p3)
            {
                memcpy(requestInfo.host, p1 + 1, p2 - p1 - 1);
                memcpy(requestInfo.port, p2 + 1, p3 - p2 - 1);
            }
            else
            {
                memcpy(requestInfo.host, p1 + 1, p3 - p1 - 1);
            }
            requestInfo.type = 1;
            printf("requestInfo.host: %s, requestInfo.port: %s\n", requestInfo.host, requestInfo.port);
            return requestInfo;
        }
    }

    // CONNECT case
    buffer = strstr(text, "CONNECT");
    if (buffer != NULL)
    {
        if(strstr(text, "Host:")==NULL){return requestInfo;} //newly added

        p1 = strchr(buffer, ' ');
        p2 = strchr(p1 + 1, ':');
        p3 = strchr(p1 + 1, ' ');

        // error case
        if (p1 == NULL || p3 == NULL)
        {
            return requestInfo;
        }

        if (p2)
        {
            memcpy(requestInfo.host, p1 + 1, p2 - p1 - 1);
            memcpy(requestInfo.port, p2 + 1, p3 - p2 - 1);
        }
        else
        {
            memcpy(requestInfo.host, p1 + 1, p3 - p1 - 1);
        }
        requestInfo.type = 2;
        printf("requestInfo.type: %d\n",requestInfo.type);
        printf("requestInfo.host: %s, requestInfo.port: %s\n", requestInfo.host, requestInfo.port);
        return requestInfo;
    }

    // CACHEREQ case
    buffer = strstr(text, "CACHEREQ");
    if (buffer != NULL)
    {
        if(strstr(text, "Host:")==NULL){return requestInfo;} //newly added
        
        p1 = strchr(buffer, ' ');
        p2 = strchr(p1 + 1, ' ');
        // error case
        if (p1 == NULL || p2 == NULL)
        {
            return requestInfo;
        }
        int urlLen = p2 - p1 - 1;
        memcpy(requestInfo.url, p1 + 1, urlLen);
        requestInfo.url[urlLen] = '\0';
        printf("requestInfo.url: %s\n", requestInfo.url);

        // extract host
        buffer = strstr(text, "Host:");
        // error case
        if (buffer == NULL)
        {
            //getHostFromUrl(requestInfo.url, requestInfo.host, requestInfo.port); //has some bugs
            return requestInfo;
        }
        else
        {
            p1 = strchr(buffer, ' ');
            p2 = strchr(p1 + 1, ':');
            p3 = strchr(p1 + 1, '\r');

            // error case
            if (p1 == NULL || p3 == NULL)
            {
                return requestInfo;
            }

            if (p2 != NULL && p2 < p3)
            {
                memcpy(requestInfo.host, p1 + 1, p2 - p1 - 1);
                memcpy(requestInfo.port, p2 + 1, p3 - p2 - 1);
            }
            else
            {
                memcpy(requestInfo.host, p1 + 1, p3 - p1 - 1);
            }
            requestInfo.type = 3;
            printf("requestInfo.host: %s, requestInfo.port: %s\n", requestInfo.host, requestInfo.port);
            return requestInfo;
        }
    }

    return requestInfo;
}

struct ResponseInfo AnalyzeResponse(char *text)
{
    struct ResponseInfo responseInfo = {0, 0, 0};
    char *buffer, *p1, *p2;
    int i = 0, j = 0;

    buffer = text;
    if ((p1 = strstr(buffer, "\r\n\r\n")) != NULL)
    {
        responseInfo.headerLength = p1 - buffer + 4; // + \r\n\r\n
        printf("headerLength: %d\n", responseInfo.headerLength);
    }

    if ((p1 = strstr(buffer, "Cache-Control")) != NULL)
    {
        // printf( "Has Cache-Control\n");
        responseInfo.hasCacheControl = 1;
        if ((p1 = strstr(buffer, "max-age")) != NULL)
        {
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

    if ((p1 = strstr(buffer, "Content-Length")) != NULL)
    {
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

void MakeKey(char *Host, char *Port, char *url, char *key)
{
    strcpy(key, "");
    strcpy(key, Host);
    strcat(key, ":");
    strcat(key, Port);
    strcat(key, "_");
    strcat(key, url);
    printf("MakeKey: %s\n", key);
}

// cooperative caching
struct ProxyList initProxyList(){
    struct ProxyList res;
    res.coCacheNum = 0;
    int i;
    for(i = 0; i < 5; i++){
        strcpy(res.host[i], "NA");
        strcpy(res.port[i], "80");
    }
    return res;
}

struct ProxyList getProxyList(){
    struct ProxyList proxyList = initProxyList();
    FILE *f = fopen("cache_server.txt", "r");
    if(f){  // if file exists
        char buf[100];
        char *p1, *p2, *p3;
        while(fgets(buf, sizeof(buf), f) != NULL){
            p1 = strchr(buf, '[');
            p2 = strchr(buf, ':');
            p3 = strchr(buf, ']');
            if(p1 == NULL && p2 == NULL && p3 == NULL){
                break;
            }
            memcpy(proxyList.host[proxyList.coCacheNum], p1 + 1, p2 - p1 - 1);
            proxyList.host[proxyList.coCacheNum][p2 - p1 - 1] = '\0';

            memcpy(proxyList.port[proxyList.coCacheNum], p2 + 1, p3 - p2 - 1);
            proxyList.port[proxyList.coCacheNum][p3 - p2 - 1] = '\0';

            proxyList.coCacheNum++;

        }
        fclose(f);
    }
    return proxyList;
}