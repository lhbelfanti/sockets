# WebServer made with C
It's a simple C application that uses the HTTP v1.1 protocol to accept a browser as a client.
It exposes a serie of endpoints thru the clients can access to static and dynamic resources. Is a console program that receives a listen port as a parameter.
The server is a multithread process, thus each request is processed on a new thread. This thread finishes and after attend the request, frees all the associated resources.
The protocol used is TCP/IPv4.
The Berkeley API is used for sockets.
