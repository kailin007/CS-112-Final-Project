#include "text_analysis.h"

// // parameter: a single line of input file, return the SingleLineInfo struct including
// // PUT(key, value maxage information) or GET(key) information
// void AnalyzeRequestSingleLine (char lineInfo[], struct RequestInfo *requestInfo){
//     char *token, *buffer, *temp;

//     buffer = (char *) malloc(MaxSingleLineLength * sizeof(char));
//     if (MaxSingleLineLength < strlen(lineInfo)){
//         lineInfo[strlen(buffer)] = '\0';
//     }
//     strcpy (buffer, lineInfo);
    
//     // printf("AnalyzeSingleLine: %s\n", lineInfo);
//     if(strstr(lineInfo, "GET") != NULL){
//         token = strtok(buffer, " ");
//         token = strtok(NULL, " ");
//         // printf( "GET:%s\n", token);
//         strcpy(requestInfo->GET, token);
//     }
//     else if(strstr(lineInfo, "Host") != NULL){
//         token = strtok(buffer, " ");
//         token = strtok(NULL, " ");
//         // printf( "%s\n", token);
//         temp = (char *) malloc(sizeof(char) * (strlen(token) + 1));
//         strcpy(temp, token);
//         if(strstr(temp, ":") != NULL){
//             // printf("1\n");
//             strcpy(requestInfo->Host_Port, temp);
//             token = strtok(temp, ":");
//             strcpy(requestInfo->Host, token);
//             token = strtok(NULL, ":");
//             requestInfo->Port = atoi(token);
//         }
//         else{
//             // printf("2\n");
//             requestInfo->Port = 80;
//             strcpy(requestInfo->Host, token);
//             strcat(requestInfo->Host_Port, token);
//             strcat(requestInfo->Host_Port, ":80");
//         }
//         // printf("Host:%s\nPort:%d\n", requestInfo->Host, requestInfo->Port);
//         free(temp);
//     }

//     free(buffer);
// }

struct RequestInfo AnalyzeRequest (char text[]){
    struct RequestInfo requestInfo = {"", "", 80};
    char *token, *buffer, *temp;
    buffer = (char *) malloc(BufferSize * sizeof(char));

    strcpy(buffer, text);
    if((temp = strstr(buffer, "GET")) != NULL){
        temp = temp + 4;
        token = strtok(temp, " ");
        printf("%s\n", token);
        strcpy(requestInfo.GET, token);
    }

    strcpy(buffer, text);
    if((temp = strstr(buffer, "Host")) != NULL){
        temp = temp + 6;
        // printf("buffer: %s\n", buffer);
        token = strtok(temp, "\r\n");
        if(strstr(token, ":") != NULL){
            // printf("1\n");
            token = strtok(token, ":");
            strcpy(requestInfo.Host, token);
            token = strtok(NULL, ":");
            requestInfo.Port = atoi(token);
        }
        else{
            // printf("2\n");
            requestInfo.Port = 80;
            strcpy(requestInfo.Host, token);
        }
    }

    printf("Host: %s\nPort: %d\n", requestInfo.Host, requestInfo.Port);
    free(buffer);


    // char *token, *buffer, **temp;
    // int i = 0,j = 0;
    // buffer = (char *) malloc(BufferSize * sizeof(char));
    // temp = (char **) malloc(MaxLine * sizeof(char *));
    // for(j = 0; j < MaxLine; j++){
    //     temp[j] = (char *) malloc(MaxSingleLineLength * sizeof(char));
    // } 
    // strcpy(buffer, text);
    // // printf("buffer in AnalyzeRequest: %s\n", buffer);
    // token = strtok(buffer, "\r\n");
    // while(token != NULL){
    //     // printf("SaveLine: %s\n", token);
    //     strcpy(temp[i], token);
    //     i++;
    //     token = strtok(NULL, "\r\n");
    //     if (i == MaxLine) break;
    // }
    
    // for(j = 0; j < i; j++){
    //     AnalyzeRequestSingleLine(temp[j], &requestInfo);
    // }

    // free(buffer);
    // free(temp);


    return requestInfo;
}


// void AnalyzeResponseSingleLine (char lineInfo[], struct ResponseInfo *responseInfo){
//     // struct SingleLineInfo singleLineInfo = {0, "", "", 0};
//     char *token, *buffer;

//     buffer = (char *) malloc(MaxSingleLineLength * sizeof(char));
//     if (MaxSingleLineLength < strlen(lineInfo)){
//         lineInfo[strlen(buffer)] = '\0';
//     }
//     strcpy (buffer, lineInfo);

//     // printf("AnalyzeResponseSingleLine: %s\n", lineInfo);
//     if(strstr(lineInfo, "Cache-Control") != NULL){
//         printf( "Has Cache-Control\n");
//         responseInfo->hasCacheControl = 1;
//         if(strstr(lineInfo, "max-age") != NULL){
//             printf( "Has max-age\n");
//             responseInfo->needCache = 1;
//         }
//         token = strtok(buffer, "=");
//         token = strtok(NULL, "=");
//         printf( "max-age:%s\n", token);
//         responseInfo->maxAge = atoi(token);
//     }

//     free(buffer);
// }


struct ResponseInfo AnalyzeResponse (char text[]){
    struct ResponseInfo responseInfo = {0, 0, 0};
    char *token, *buffer, *temp;
    int i = 0,j = 0;
    buffer = (char *) malloc(BufferSize * sizeof(char));

    memcpy(buffer, text, BufferSize);
    if((temp = strstr(buffer, "\n")) != NULL){
        responseInfo.headerLength = temp - buffer + 1; // + 1 = + \n
        printf("headerLength: %d\n", responseInfo.headerLength);
    }

    memcpy(buffer, text, BufferSize);
    if((temp = strstr(buffer, "Cache-Control")) != NULL){
        // printf( "Has Cache-Control\n");
        responseInfo.hasCacheControl = 1;
        if((temp = strstr(buffer, "max-age")) != NULL){
            // printf( "Has max-age\n");
            responseInfo.needCache = 1;
            token = strtok(temp, "\r\n");
            token = strtok(temp, "=");
            token = strtok(NULL, "=");
            // printf( "max-age:%s\n", token);
            responseInfo.maxAge = atoi(token);
        }
    }

    memcpy(buffer, text, BufferSize);
    if((temp = strstr(buffer, "Content-Length")) != NULL){
        token = strtok(temp, "\r\n");
        token = strtok(temp, ":");
        token = strtok(NULL, ":");
        // printf( "max-age:%s\n", token);
        responseInfo.contentLength = atoi(token);
        // printf("Content-Length: %d\n", responseInfo.contentLength);
    }
    free(buffer);

    // struct ResponseInfo responseInfo = {0, 0, 0};
    // char *token, *buffer, **temp;
    // int i = 0,j = 0;
    // buffer = (char *) malloc(BufferSize * sizeof(char));
    // temp = (char **) malloc(MaxLine * sizeof(char *));
    // for(j = 0; j < MaxLine; j++){
    //     temp[j] = (char *) malloc(MaxSingleLineLength * sizeof(char));
    // } 
    // strcpy(buffer, text);

    // token = strtok(buffer, "\r\n");
    // while(token != NULL){
    //     // printf("SaveLine: %s\n", token);
    //     strcpy(temp[i], token);
    //     i++;
    //     token = strtok(NULL, "\r\n");
    //     if (i == MaxLine) break;
    // }
    // for(j = 0; j < i; j++){
    //     AnalyzeResponseSingleLine(temp[j], &responseInfo);
    // }

    // free(buffer);
    // free(temp);

    // printf("\nhasCacheControl: %d, needCache: %d, maxAge: %d\n", responseInfo.hasCacheControl, responseInfo.needCache, responseInfo.maxAge);
    return responseInfo;
}