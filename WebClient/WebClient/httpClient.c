//
//  httpClient.c
//  WebClient
//
//  Created by Lucas Belfanti on 10/30/17.
//  Copyright © 2017 Lucas Belfanti. All rights reserved.
//

#include "httpClient.h"

#define CHUNK_SIZE 1024

/**
 * Get arguments from command line
 */
PAPP_ARGUMENTS getProgramArguments(char **arguments) {
    
    PAPP_ARGUMENTS pArguments = malloc(sizeof(APP_ARGUMENTS));
    pArguments->address = arguments[1];
    pArguments->port    = arguments[2];
    
    return pArguments;
}

/**
 * Display help
 */
VOID displayUsage() {
    
    fprintf(stdout, "Usage: client [host] [port]\n");
    fprintf(stdout, "EXAMPLES:\n");
    fprintf(stdout, "%-20s\n", "client www.google.com 80");
}

/**
 * Entry point
 */
int main(int argc, char * argv[])
{
    int res;                                        // Result of each call
    struct addrinfo     hints;                      // Template
    struct addrinfo     *serverInfo = NULL;         // Results
    struct addrinfo     *targetServerInfo = NULL;   // Target
    PAPP_ARGUMENTS      arguments;                  // Arguments
    
    // Arguments
    if(argc < 3)
    {
        displayUsage();
        return 0;
    }
    else
    {
        arguments = getProgramArguments(argv);
    }
    
    // Prepare template server info struct
    memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    
    res = getaddrinfo(arguments->address, arguments->port, &hints, &serverInfo);
    
    if (res != 0)
    {
        free(arguments);
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(res));
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Structures obtained (addrinf, each of which contains an Internet address) -- getaddrinfo()\n");
    
    // ServerInfo now points to a linked list of 1 or more struct addrinfo
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
    
    // Connect to remote server
    INT cxSocket = socket(targetServerInfo->ai_family,
                          targetServerInfo->ai_socktype,
                          targetServerInfo->ai_protocol);
    
    
    res = connect(cxSocket, targetServerInfo->ai_addr, targetServerInfo->ai_addrlen);
    
    if(res == -1)
    {
        freeaddrinfo(serverInfo);
        close(cxSocket);
        fprintf(stderr, "Could not connect -- connect()\n");
        exit(1);
    }
    
    fprintf(stderr, "Connected -- connect()\n");
    
    // Prepare http request
    PCHAR httpHeader    = "GET /sample.jpg HTTP/1.1\r\nHost: %s\r\n\r\n";
    ULONG httpRequestSize = strlen(httpHeader) + strlen(arguments->address);
    PCHAR httpRequest   = malloc(httpRequestSize);
    memset(httpRequest, 0, httpRequestSize);
    sprintf(httpRequest, httpHeader, arguments->address);
    
    // Send request
    ssize_t rxBytes, cxBytes, rxTotal = 0;
    
    cxBytes = send(cxSocket, httpRequest, httpRequestSize, 0);
    printf("REQUEST\n%s\n%lu bytes sended.\n\n", httpRequest, cxBytes);
    
    // Receive response
    PCHAR chunk = NULL;
    // Init with chunk size
    chunk = malloc(CHUNK_SIZE);
    memset(chunk, 0, CHUNK_SIZE);
    
    do
    {
        rxBytes = recv(cxSocket, chunk, CHUNK_SIZE, 0);
        
        if(rxBytes == -1)
        {
            fprintf(stderr, "Recieve error -- recv()\n");
            break;
        }
        
        if(rxBytes == 0)
        {
            fprintf(stderr, "Connection closed by peer -- rxBytes == 0\n");
            break;
        }
        
        rxTotal = rxTotal + rxBytes;
        
        printf("\n-------------------------\n");
        printf("Server response: %s", chunk);
        printf("\n-------------------------\n");
        memset(chunk, 0, CHUNK_SIZE);
        
    } while(TRUE);
    
    printf("bytes: %lu\n", rxTotal);
    
    // Dealloc memory and finish
    close(cxSocket);
    free(chunk);
    free(httpRequest);
    free(arguments);
    freeaddrinfo(serverInfo);
    return 0;
}
