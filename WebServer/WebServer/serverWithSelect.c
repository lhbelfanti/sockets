//
//  serverWithSelect.c
//  WebServer
//
//  Created by Lucas Belfanti on 9/25/17.
//  Copyright © 2017 Lucas Belfanti. All rights reserved.
//

#include "serverWithSelect.h"

#define MAX_PENDING_CONNECTIONS 10 // maximum length to which the queue of pending connections for sockfd (socket file descriptor) may grow

/**
 * Get arguments from command line
 */
PAPP_ARGUMENTS getProgramArguments(char **arguments)
{
    PAPP_ARGUMENTS pArguments = malloc(sizeof(APP_ARGUMENTS));
    pArguments->port    = arguments[1];
    
    return pArguments;
}

/**
 * Display help
 */
VOID displayUsage()
{
    fprintf(stdout, "Usage: [port]\n");
    fprintf(stdout, "EXAMPLES:\n");
    fprintf(stdout, "%-20s\n", "http (or PORT)");
}

/**
 * Entry point
 */
INT main(int argc, char * argv[])
{
    int res;                                        // result of each call
    struct addrinfo     hints;                      // template
    struct addrinfo     *serverInfo = NULL;         // results
    struct addrinfo     *targetServerInfo = NULL;   // target
    PAPP_ARGUMENTS      arguments;                  // arguments
    
    // arguments
    if(argc < 2)
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
    
    int yes=1;
    setsockopt(cxSocket, SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(yes));
    
    
    if (cxSocket == -1)
    {
        freeaddrinfo(serverInfo);
        close(cxSocket);
        free(arguments);
        fprintf(stderr, "Could not create socket\n");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Socket created -- socket()\n");
    
    // get socket options
    unsigned int bufferSize = 0;
    unsigned int bufferSizeLenght = sizeof(bufferSize);
    
    getsockopt(cxSocket, SOL_SOCKET, SO_RCVBUF, &bufferSize, &bufferSizeLenght);
    printf("recv buffer size: %u\n", bufferSize);
    
    getsockopt(cxSocket, SOL_SOCKET, SO_SNDBUF, &bufferSize, &bufferSizeLenght);
    printf("send buffer size: %u\n", bufferSize);
    
    getsockopt(cxSocket, SOL_SOCKET, SO_REUSEADDR, &bufferSize, &bufferSizeLenght);
    printf("reuseaddr: %u\n", bufferSize);
    
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
    
    //------------------------------------------------------------------------
    // Select(
    
    struct timeval tv;
    
    // A file descriptor is considered ready for reading if a read call will not block.
    // This usually includes the read offset being at the end of the file or there is
    // an error to report. A server socket is considered ready for reading if there is
    // a pending connection which can be accepted with accept
    fd_set readfds;
    fd_set tempreadfds;
    
    int fdmax = cxSocket;
    
    // Specifies the minimum interval that select() should block waiting for a file descriptor to become ready.
    // If both fields of the timeval structure are zero, then select() returns immediately.
    // If timeout is NULL (no timeout), select() can block indefinitely.
    tv.tv_sec = 2;
    tv.tv_usec = 500000;
    
    FD_ZERO(&readfds);
    FD_SET(cxSocket, &readfds);
    
    
    while(TRUE)
    {
        tempreadfds = readfds;

        int ret = select(fdmax + 1, &tempreadfds, NULL, NULL, &tv); //&tv);
        
        fprintf(stdout, "select() return: %d.\n", ret);
        
        if (ret == -1)
        {
            freeaddrinfo(serverInfo);
            close(cxSocket);
            free(arguments);
            fprintf(stderr, "Could not create select -- select()\n");
            exit(EXIT_FAILURE);
        }
        
        for(int i = 0; i <= fdmax; i++)
        {
            if(FD_ISSET(i, &tempreadfds))
            {
                fprintf(stdout, "\nselect() return: %d OK\n", i);
                fprintf(stdout, "Socket with the fd flag active: %d\n, ", i);
                
                if(i == cxSocket)
                {
                    struct sockaddr sockaddClient;
                    socklen_t sockaddClientLength;
                    
                    INT socketNewCx = accept(cxSocket, &sockaddClient, &sockaddClientLength);
                    
                    if (socketNewCx == -1)
                    {
                        close(socketNewCx);
                        fprintf(stderr, "Could not accept connection -- accept()\n");
                        continue;
                    }
                    
                    fprintf(stderr, "Connection accepted -- accept()\n");
                    
                    FD_SET(socketNewCx, &readfds);
                    
                    if(socketNewCx > fdmax)
                    {
                        fdmax = socketNewCx;
                    }
                    
                    char *str = randstring(10);
                    ssize_t msgBytes = strlen(str);
                    send(socketNewCx, str, msgBytes, 0);
                    fprintf(stdout, "String send to client: %s\n", str);
                    
                }
                else
                {
                    char buffer[256];
                    int ret = recv(i, &buffer, 256, 0);
                    fprintf(stdout, "Client response: %s\n", buffer);
                    
                    if(ret == 0)
                    {
                        FD_CLR(i, &readfds);
                        continue;
                    }
                }
            }
            else
            {
                fprintf(stdout, "%d, ", i);
            }
        }
        fprintf(stdout, "\n");
    }
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
