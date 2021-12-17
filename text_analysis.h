#ifndef TEXT_ANALYSIS_ /* Include guard */
#define TEXT_ANALYSIS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MaxSingleLineLength 3000

struct RequestInfo
{
    int type; //-1: error; 1: GET; 2: CONNECT; 3: CACHEREQ.
    char url[MaxSingleLineLength];
    char host[MaxSingleLineLength];
    char port[10];
};

struct ResponseInfo
{
    int hasCacheControl; // 0: false; 1: true.
    int needCache;       // 0: false; 1: true.
    int maxAge;          // in seconds; default 3600
    int contentLength;
    int headerLength;
};

struct ProxyList{
    int coCacheNum;
    char host[5][100];
    char port[5][10];
    int socket[5];
};

struct RequestInfo AnalyzeRequest(char *text);
struct ResponseInfo AnalyzeResponse(char *text);
void MakeKey(char *Host, char *Port, char *url, char *key);
struct ProxyList getProxyList();

#endif // FILE_ANALYSIS_