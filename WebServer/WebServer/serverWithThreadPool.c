//
//  serverWithThreadPool.c
//  WebServer
//
//  Created by Lucas Belfanti on 10/29/17.
//  Copyright © 2017 Lucas Belfanti. All rights reserved.
//

#include "serverWithThreadPool.h"

#define MAX_PENDING_CONNECTIONS 10 // maximum length to which the queue of pending connections for sockfd (socket file descriptor) may grow
/**
 * Get arguments from command line
 */
PAPP_ARGUMENTS getProgramArguments(char **arguments)
{
    PAPP_ARGUMENTS pArguments = malloc(sizeof(APP_ARGUMENTS));
    pArguments->port         = arguments[1];
    pArguments->thread_count = arguments[2];
    pArguments->queue_size   = arguments[3];
    
    return pArguments;
}

/**
 * Display help
 */
VOID displayUsage()
{
    fprintf(stdout, "Usage: [port] [threads]\n");
    fprintf(stdout, "EXAMPLES:\n");
    fprintf(stdout, "%-20s\n", "http (or PORT) threads quantity");
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
    if(argc < 3)
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
    threadpool_t *pool = threadpool_create((int)arguments->thread_count, (int)arguments->queue_size, 0);
    
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
        
        threadpool_add(pool, &runnable, params, 0);
    }
    
    threadpool_destroy(pool, 0);
}

VOID runnable(PVOID params)
{
    PTHREAD_PARAMS tParams = params;
    ssize_t sendBytes, msgBytes;
    char *str = randstring(10);
    msgBytes = strlen(str);
    sendBytes = send(tParams->fd, str, msgBytes, 0);
    close(tParams->fd);
    free(params);
}

PCHAR randstring(size_t length)
{
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
    char *randomString = NULL;
    
    if (length) {
        randomString = malloc(sizeof(char) * (length +1));
        
        if (randomString) {
            for (int n = 0;n < length;n++) {
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }
            
            randomString[length] = '\0';
        }
    }
    
    return randomString;
}
