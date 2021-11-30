#include "client.h"

#define BUFSIZE 1048576
#define h_addr h_addr_list[0]

/* 
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

/* 
fail prompt: "ERROR, no such proxy exists!"
success prompt: "Successfully connect to proxy!"
*/
char *one_client_makes_connection_to_proxy(DATA* data)
{   
    char* hostname = data->hostname;
    struct hostent *server = data->server;
    int portno = data->portno;
    int sockfd,  n;
    struct sockaddr_in serveraddr;
    char *stat;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (server == NULL)
    {   
        stat = "ERROR, no such proxy exists!";
        return stat;
    }
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
    {
        stat = "ERROR, no such proxy exists!";
        return stat;
    };

    stat = "Successfully connect to proxy!";

    close(sockfd);
    return stat;
}

/*
fail prompt: "At least one client failed to connect to proxy"
success prompt: "All clients successfully connect to proxy!"
test_condition: "make connections one by one"/"make connections at the same time"
*/
char *multiple_clients_make_connections_to_proxy(int client_num, char *hostname, int portno, char *test_condition)
{
    char *stat;
    struct hostent *server;
    server = gethostbyname(hostname);
    DATA data;
    data.hostname = hostname;
    data.portno = portno;
    data.server = server;

    if (!strcmp(test_condition, "make connections one by one"))
    {
        for (int i = 0; i < client_num; i++)
        {
            stat = one_client_makes_connection_to_proxy(&data);
            if (strcmp(stat, "Successfully connect to proxy!") != 0)
            {
                return "At least one client failed to connect to proxy";
            }
        }
        return "All clients successfully connect to proxy!";
    }
    else
    {       
           pthread_t threads[client_num];
           char* stat[client_num];
             
           for (int i = 0; i < client_num; i++)
           {
               pthread_create(&threads[i], NULL, one_client_makes_connection_to_proxy, &data); 
           }
           for (int i = 0; i < client_num; i++)
           {
               pthread_join(threads[i], &stat[i]);
           }

           for(int i = 0; i < client_num; i++){
            if (strcmp(stat[i], "Successfully connect to proxy!") != 0)
            {
                return "At least one client failed to connect to proxy";
            }
           }
        
        return "All clients successfully connect to proxy!";
    }
}

/*
fail prompts: "failed to make first connection to proxy"/"failed to make connection again to proxy after client exited last time"
success prompt: "client exited without killing the proxy"
exist_condition: "before sending request"/"after sending request"/"after receiving message"
*/
char *client_exit(char *request, char *hostname, int portno, char *exist_condition)
{
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *stat;
    char buf[BUFSIZE];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        stat = "failed to make first connection to proxy";
        return stat;
    }
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
    {
        stat = "failed to make first connection to proxy";
        return stat;
    };

    if (!strcmp(exist_condition, "after sending request"))
    {   
        int n = write(sockfd, request, strlen(request));
    }
    if (!strcmp(exist_condition, "after receiving message"))
    {   
        bzero(buf, BUFSIZE);
        int n = write(sockfd, request, strlen(request));
        n = read(sockfd, buf, BUFSIZE);
    }
    
    close(sockfd);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        stat = "failed to make connection again to proxy after client exited last time";
        return stat;
    }
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
    {
        stat = "failed to make connection again to proxy after client exited last time";
        return stat;
    };

    close(sockfd);
    return "client exited without killing the proxy";
}

/*
fail prompts: "ERROR, no such proxy exists!"/"Proxy responded 'Bad Request!'"/"Proxy closed the connection because an error occur when forwarding"
success prompt: "Successfully get correct respond from server"
*/
char *one_client_sends_request_to_server(ReqDATA* data)
{  
    char* request = data->request;
    char* hostname = data->hostname;
    struct hostent *server = data->server;
    int portno = data->portno;
    int sockfd,  n;
    struct sockaddr_in serveraddr;
    char *stat;
    char* buf = (char*)malloc(BUFSIZE);
    char* str = (char*)malloc(strlen("Bad Request!"));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (server == NULL)
    {
        stat="ERROR, no such proxy exists!";
        return stat;
    }

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
    {
        stat="ERROR, no such proxy exists!";
        return stat;
    };

    bzero(buf, BUFSIZE);
    n = write(sockfd, request, strlen(request));
    if (n < 0)
        error("ERROR writing to socket");

    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);

    memcpy(str, buf, strlen("Bad Request!"));
    if (n > 0 && (!strcmp(str, "Bad Request!")))
    {
        stat="Proxy responded 'Bad Request!'";
        close(sockfd);
        return stat;
    }
    if (n <= 0)
    {
        stat="Proxy closed the connection because an error occur when forwarding";
        close(sockfd);
        return stat;
    }

    stat="Successfully get correct respond from server";
    close(sockfd);
    return stat;
}

/*
fail prompts: "ERROR, no such proxy exists!"/"Proxy closed the connection because error(s) occur when forwarding"
success prompt: "All requests get correct respond from server"
send_condition: "concurrent"/"one by one"
*/
char *multiple_clients_send_requests_to_server(int client_num, char *requests[client_num], char *hostname, int portno, char *send_condition)
{
    struct hostent *server;
    server = gethostbyname(hostname);
    ReqDATA data;
    data.hostname = hostname;
    data.portno = portno;
    data.server = server;

    if (!strcmp(send_condition, "one by one"))
    {   //printf("******************************************************************************************************************8\n");
        char* stat;
       
        for(int i=0; i<client_num; i++)
        {
            data.request = requests[i];
            stat = one_client_sends_request_to_server(&data);
            //printf("one by one status %d: %s\n",i,stat);
            if (strcmp("Successfully get correct respond from server", stat) != 0)
            {
                if(!strcmp("ERROR, no such proxy exists!",stat)){return "ERROR, no such proxy exists!";}
                else{return "Proxy closed the connection because error(s) occur when forwarding";}
            }
        }
    }
    else
    {
        printf("---------------------------------------------------------------------------------------------------------------------\n");
        pthread_t threads[client_num];
        char* status[client_num];
            
        for (int i = 0; i < client_num; i++)
        {   
            data.request = requests[i];
            pthread_create(&threads[i], NULL, one_client_sends_request_to_server, &data); 
        }
        for (int i = 0; i < client_num; i++)
        {
            pthread_join(threads[i], &status[i]);
            printf("thread: %d\n", threads[i]);
            printf("concurrent status %d: %s\n",i,status[i]);
        }

        for(int i = 0; i < client_num; i++){
            if (strcmp("Successfully get correct respond from server", status[i]) != 0)
            {
                if(!strcmp("ERROR, no such proxy exists!", status[i])){return "ERROR, no such proxy exists!";}
                else{return "Proxy closed the connection because error(s) occur when forwarding";}
            }
        }
    }
    
    return "All requests get correct respond from server";
}