<p align="center">
  <img src="media/sockets-logo.png" width="100" alt="Repository logo" />
</p>
<h3 align="center">Sockets</h3>
<p align="center">Simple client-server application, written in C<p>
<p align="center">
    <img src="https://img.shields.io/github/repo-size/lhbelfanti/sockets?label=Repo%20size" alt="Repo size" />
    <img src="https://img.shields.io/github/license/lhbelfanti/sockets?label=License" alt="License" />
</p>

---
# Client-Server

This is a simple client-server application, written in C.

The client can access to static and dynamic resources through the endpoints exposed by the server.

The server is multithreaded. Each request is processed in a new thread. After the thread finishes the processing, all the associated resources are freed.

## Technical information

- It is a console program that receives a listen port as a parameter.
- The solution uses the HTTP v1.1 protocol to accept a browser as a client.
- The protocol used is TCP/IPv4.
- The Berkeley API is used for sockets.
