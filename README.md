# CS112 final project, high performance HTTP proxy.  
Authors: Keren Zhou (kzhou), Ruiyuan Gu (rgu03)  
Creation date: 2021-11-10  

# Usage

## Compile.
```
$ make  
```
## Run proxy in default (SSL tunneling) mode.
Run proxy on <port>.  
```
$ ./proxy <port>  
```
where &lt;port&gt; is the port number that the proxy listens on, which is customizable.  

## Run proxy in SSL interception mode.
```
$ ./proxy <port> cert.pem key.pem  
```
where cert.pem and key.pem are certificate and private key files in PEM format. They are used in SSL interception.  

## Test proxy with curl.  
TODO

## Test proxy with browser.  
TODO


# Additional Functionalities
* Support HTTP response using chunked transfer encoding.
* SSL interception (implemented with best effort)


# Notes
The current version of proxy partially implemented SSL interception. It can pass curl tests but it has bugs to work with browser.  
The proxy under no-ssl directory haven't implemented SSL interception. It can work with browser properly.  


# Files
* proxy.c: Main driver for the proxy.  
* cache.h/.c: Cache module. We cache full server response using hostname + url as the key.  
* sock_buf.h/.c: Socket buffer module. Each socket buffer buffers data received from each socket. It also contains other info for the socket, such as whether the socket is for a client or a server, whether the socket is on either end of a SSL connection, etc.
* http_utils.h/.c: Utilities for HTTP. It contains parser for HTTP request and response.
* logger.h/.c: Log utility. It can print user-defined message with filename and line number.
* cert.pem: Self-signed certificate for SSL interception.
* key.pem: Private key for SSL interception.
