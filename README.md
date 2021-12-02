# CS112 final project, high performance HTTP proxy.  
Authors: Keren Zhou (kzhou), Ruiyuan Gu (rgu03)  
Creation date: 2021-11-10  

# Usage

## Start proxy.
Compile proxy.  
```
$ make  
```
Run proxy on port 9160.  
```
$ ./proxy 9160  
```
The port number can be customized.  

## Run curl test.  
After running proxy on port 9160, we can run curl test.  
If you use other port, you should modify curl_test.sh by setting variable proxy to 'localhost:<your_port>'.  
```
$ bash curl_test.sh  
```
The curl tests compare the responses of direct access and via proxy by command line tool diff.  
You can check curl outputs in *.txt files.  
After curl test, you can remove all the *.txt files to cleanup.  

## Run with browser.  
After starting the proxy, set system proxy to "localhost:<port>" or use some browser extension.  
Here, we use Chrome extension SwitchyOmega and set the proxy to "localhost:9160".  


# Additional Functionalities
* Support HTTP response using chunked transfer encoding.
* SSL interception (partially implemented with best effort)


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
