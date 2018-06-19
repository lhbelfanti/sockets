//
//  httpServer.c
//  WebServer
//
//  Created by Lucas Belfanti on 10/30/17.
//  Copyright © 2017 Lucas Belfanti. All rights reserved.
//

#include "httpServer.h"

#define MAX_PENDING_CONNECTIONS 10 // maximum length to which the queue of pending connections for sockfd (socket file descriptor) may grow
#define CHUNK_SIZE 1024

/**
 * Get arguments from command line
 */
PAPP_ARGUMENTS getProgramArguments(char **arguments)
{
    PAPP_ARGUMENTS pArguments = malloc(sizeof(APP_ARGUMENTS));
    pArguments->port         = arguments[1];
    pArguments->thread_count = arguments[2];
    pArguments->queue_size   = arguments[3];
    pArguments->file_path    = arguments[4];
    
    return pArguments;
}

/**
 * Display help
 */
VOID displayUsage()
{
    fprintf(stdout, "Usage: [port] [threads_count] [queue_size] [file_path]\n");
    fprintf(stdout, "EXAMPLES:\n");
    fprintf(stdout, "%-20s\n", "http (or PORT) 10 5 .../sample.jpg");
}

/**
 * Entry point
 */
INT main(int argc, char * argv[])
{
    int res;                                        // result of each call
    int clientLen;                                  // lenght of clientAddress
    struct addrinfo     hints;                      // template
    struct addrinfo     *serverInfo = NULL;         // results
    struct addrinfo     *targetServerInfo = NULL;   // target
    struct sockaddr_in  clientAddress;              // client address
    PAPP_ARGUMENTS      arguments;                  // arguments
    
    // arguments
    if(argc < 4)
    {
        displayUsage();
        return 0;
    }
    else
    {
        arguments = getProgramArguments(argv);
    }
    
    // prepare template server info struct
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    
    res = getaddrinfo(NULL, arguments->port, &hints, &serverInfo);
    
    if (res != 0)
    {
        free(arguments);
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(res));
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Structures obtained (addrinf, each of which contains an Internet address) -- getaddrinfo()\n");
    
    // serverInfo now points to a linked list of 1 or more struct addrinfo
    
    // iterate over all serverinfo results
    for (struct addrinfo *currentAddr = serverInfo; currentAddr != NULL; currentAddr = currentAddr->ai_next)
    {
        char ipAddress[100];
        void *rawAddress = NULL;
        
        if(currentAddr->ai_family == AF_INET)
        {
            rawAddress = &((struct sockaddr_in *) currentAddr->ai_addr)->sin_addr;
            
            if(targetServerInfo == NULL)
            {
                targetServerInfo = currentAddr;
            }
            
        }
        else
        {
            rawAddress = &((struct sockaddr_in6 *) currentAddr->ai_addr)->sin6_addr;
        }
        
        printf("Server IP: %s\n", inet_ntop(currentAddr->ai_family,
                                            rawAddress,
                                            ipAddress,
                                            sizeof(ipAddress)));
    }
    
    INT cxSocket = socket(targetServerInfo->ai_family,
                          targetServerInfo->ai_socktype,
                          targetServerInfo->ai_protocol);
    
    int option = 1;
    setsockopt(cxSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    if (cxSocket == -1)
    {
        freeaddrinfo(serverInfo);
        close(cxSocket);
        free(arguments);
        fprintf(stderr, "Could not create socket\n");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Socket created -- socket()\n");
    
    res = bind(cxSocket,
               targetServerInfo->ai_addr,
               targetServerInfo->ai_addrlen);
    
    if(res == -1)
    {
        freeaddrinfo(serverInfo);
        close(cxSocket);
        free(arguments);
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Address assigned to socket -- bind()\n");
    
    res = listen(cxSocket,
                 MAX_PENDING_CONNECTIONS);
    
    if(res == -1)
    {
        freeaddrinfo(serverInfo);
        close(cxSocket);
        free(arguments);
        fprintf(stderr, "Listen error\n");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Socket marked as passive, to accept incoming connections -- listen()\n");
    
    fprintf(stdout, "Waiting for incoming connections...\n");
    
    clientLen = sizeof(clientAddress);
    
    // Create pool
    threadpool_t *pool = threadpool_create(atoi(arguments->thread_count), atoi(arguments->queue_size), 0);
    fprintf(stderr, "Pool started with %d threads and queue size of %d\n", atoi(arguments->thread_count), atoi(arguments->queue_size));
    
    while (TRUE)
    {
        INT cliSocket = accept(cxSocket,
                               (struct sockaddr *) &clientAddress,
                               (socklen_t*) &clientLen);
        
        if (cliSocket == -1)
        {
            freeaddrinfo(serverInfo);
            close(cliSocket);
            close(cxSocket);
            free(arguments);
            fprintf(stderr, "Could not accept connection -- accept()\n");
            exit(EXIT_FAILURE);
        }
        
        fprintf(stderr, "Connection accepted -- accept()\n");
        
        PTHREAD_PARAMS params = malloc(sizeof(THREAD_PARAMS));
        params->fd = cliSocket;
        params->file_path = arguments->file_path;
        
        printf("\nSocket: %d, filePath: %s\n", params->fd, params->file_path);
        
        threadpool_add(pool, &runnable, params, 0);
    }
    
    //The code shouldn't reach here. But it's the place where it should be destroyed.
    //threadpool_destroy(pool, 0);
}

VOID runnable(PVOID params)
{
    sleep(5);
    PTHREAD_PARAMS tParams = params;
    getRequest(tParams);
}

VOID getRequest(PTHREAD_PARAMS params)
{
    ssize_t rxBytes, rxTotal = 0;
    // Receive request and parse it
    PCHAR chunk = NULL;
    // Init with chunk size
    chunk = malloc(CHUNK_SIZE);
    memset(chunk, 0, CHUNK_SIZE);
    
    do
    {
        rxBytes = recv(params->fd, chunk, CHUNK_SIZE, 0);
        
        if(rxBytes == -1)
        {
            fprintf(stderr, "Recieve error -- recv()\n");
            break;
        }
        
        if(rxBytes == 0)
        {
            fprintf(stderr, "Connection closed by peer\n");
            break;
        }
        
        rxTotal = rxTotal + rxBytes;
        
        /*printf("\n-------------------------\n");
        printf("Client Request:\n %s", chunk);
        printf("\n-------------------------\n");*/
        parseRequest(params, chunk);
        memset(chunk, 0, CHUNK_SIZE);
    } while(TRUE);
}

VOID parseRequest(PTHREAD_PARAMS params, PCHAR request)
{
    PCHAR httpMethod = malloc(sizeof(PCHAR) * 10);
    PCHAR resourcePath = malloc(sizeof(PCHAR) * 100);
    
    PCHAR start = request;
    PCHAR end;
    size_t pathLen;
    
    /* Verify that there is a 'GET ' at the beginning of the string. */
    if(strncmp("GET ", start, 4))
    {
        fprintf(stderr, "Parse error: 'GET ' is missing.\n");
        fprintf(stderr, "\nMalformed REQUEST -> %s\n", request);
        return;
    }
    
    /* Copy the path string to the path storage. */
    memcpy(httpMethod, start, 3);
    
    /* Set the start pointer at the first character beyond the 'GET '. */
    start += 4;
    
    /* From the start position, set the end pointer to the first white-space character found in the string. */
    end = start;
    while(*end && !isspace(*end))
        ++end;
    
    /* Calculate the path length, and allocate sufficient memory for the path plus string termination. */
    pathLen = (end - start);
    resourcePath = malloc(pathLen + 1);
    if(resourcePath == NULL)
    {
        fprintf(stderr, "malloc() failed. \n");
        return;
    }
    
    /* Copy the path string to the path storage. */
    memcpy(resourcePath, start, pathLen);
    
    /* Terminate the string. */
    resourcePath[pathLen] = '\0';
    
    //printf("HTTP Method: %s resoucePath: %s\n", httpMethod, resourcePath);
    
    params->resource = resourcePath;
    
    if (strncmp(httpMethod, "GET", 3) == 0)
    {
        if (strncmp(resourcePath, "/favicon.ico", strlen(resourcePath)) == 0)
            createResponse(params, "image/x-icon");
        else
            createResponse(params, "image/jpeg");
    }
    else
    {
        fprintf(stderr, "\nMalformed REQUEST -> %s\n", request);
    }
}

VOID createResponse(PTHREAD_PARAMS params, PCHAR mimeType)
{
    ULONG fsize;
    
    char *result = malloc(strlen(params->file_path) + strlen(params->resource)+1);//+1 for the null-terminator
    strcpy(result, params->file_path);
    strcat(result, params->resource);
    
    FILE *pfile = fopen(result, "r+");
    if (pfile == NULL)
    {
        fprintf(stderr, "\nFile not found! -> %s\n", params->file_path);
        return;
    }
    else
    {
        fseek(pfile, 0, SEEK_END);
        fsize = ftell(pfile);
        fseek(pfile, 0, SEEK_SET);
        rewind(pfile);
        //printf("\nFile contains %ld bytes!\n", fsize);
    }
    
    PCHAR buffer = NULL;
    ssize_t bytesSent = 0;
    LONG ret = 0;
    ULONG bytesCounter = 0, totalBytes = 0;
    
    PCHAR httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n";
    ULONG httpResponseSize = strlen(httpHeader);
    PCHAR httpResponse = malloc(httpResponseSize + strlen(mimeType) + fsize);
    memset(httpResponse, 0, httpResponseSize);
    sprintf(httpResponse, httpHeader, mimeType, fsize);
    
    bytesSent = send(params->fd, httpResponse, strlen(httpResponse), 0);
    //printf("\n\nRESPONSE:\n%s%lu bytes sent. Missing: %lu\n\n", httpResponse, bytesSent, strlen(httpResponse) - bytesSent);
    
    totalBytes = fsize;
    
    buffer = malloc(CHUNK_SIZE);
    memset(buffer, 0, CHUNK_SIZE);
    //printf("\nBytes to send: %lu\n\n", fsize);
    /* Sending file data */
    while ((ret = fread(buffer, 1, CHUNK_SIZE, pfile)) > 0)
    {
        bytesSent = send(params->fd, buffer, ret, 0);
        totalBytes = totalBytes - bytesSent;
        bytesCounter = bytesCounter + bytesSent;
        //printf("RESPONSE:\n%s\n\n%lu bytes sent. %ld/%ld\n\n", buffer, bytesSent, bytesCounter, fsize);
        memset(buffer, 0, CHUNK_SIZE);
    }

    printf("File descriptor finished: %d\n", params->fd);
    fclose(pfile);
}
