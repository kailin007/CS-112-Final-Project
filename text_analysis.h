#ifndef TEXT_ANALYSIS_   /* Include guard */
#define TEXT_ANALYSIS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MaxSingleLineLength 200
#define BufferSize 10485760
#define MaxLine 1000

struct RequestInfo{
    char GET[MaxSingleLineLength];
    char Host[MaxSingleLineLength];
    int Port;
};

struct ResponseInfo{
    int hasCacheControl; // 0: false; 1: true.
    int needCache;  // 0: false; 1: true.
    int maxAge; //in seconds; default 3600
    int contentLength;
    int headerLength;
};

// void AnalyzeRequestSingleLine (char lineInfo[], struct RequestInfo *requestInfo);
struct RequestInfo AnalyzeRequest (char text[]);

// void AnalyzeResponseSingleLine (char lineInfo[], struct ResponseInfo *responseInfo);
struct ResponseInfo AnalyzeResponse (char text[]);

#endif // FILE_ANALYSIS_