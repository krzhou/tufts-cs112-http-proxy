# CS112 final project, high performance HTTP proxy.  
Authors: Keren Zhou (kzhou), Ruiyuan Gu (rgu03)  
Creation date: 2021-11-10  

# Usage
## Compile.
Open `Makefile` and set variable `PORT` to the port number you want the proxy listens on.  
&nbsp;

Then, compile with:
```
$ make
```
&nbsp;

To cleanup, run:
```
$ make clean
```
&nbsp;


## Run proxy in SSL tunnel (default) mode.
Run proxy on <port>.  
```
$ ./proxy <port>
```
where &lt;port&gt; is the port number that the proxy listens on, which is customizable.  
&nbsp;


## Run proxy in SSL interception mode.
```
$ ./proxy <port> cert.pem key.pem  
```
where cert.pem and key.pem are certificate and private key files in PEM format. They are used in SSL interception.  

## Run integration test.  
Test SSL tunnel mode individually:
```
$ python3 test_proxy_default.py [port]
```
where `port` is the port number that the proxy listens on, 9999 by default.  
&nbsp;

Test SSL interception mode individually:
```
$ python3 test_proxy_ssl_interception.py [port]
```
&nbsp;


## Run benchmark of page load time.  
Bench SSL tunnel mode:
```
$ python3 bench_proxy_default.py [port]
```
&nbsp;

Bench SSL interception mode:
```
$ python3 bench_proxy_ssl_interception.py [port]
```
&nbsp;


# Files
* proxy.c: Main driver for the proxy.
* cache.h/.c: Cache module. We cache full server response using hostname + url as the key.
* sock_buf.h/.c: Socket buffer module. Each socket buffer buffers data received from each socket. It also contains other info for the socket, such as whether the socket is for a client or a server, whether the socket is on either end of a SSL connection, etc.
* http_utils.h/.c: Utilities for HTTP. It contains parser for HTTP request and response.
* logger.h/.c: Log utility. It can print user-defined message with filename and line number.
* cert.pem: Self-signed certificate for SSL interception.
* key.pem: Private key for SSL interception.
* test_proxy_default.py: Integration test for proxy in SSL tunnel mode.
* test_proxy_ssl_interception.py: Integration test for proxy in SSL interception mode.
* bench_proxy_default.py: Page load time benchmark for proxy in SSL tunnel mode.
* bench_proxy_ssl_interception.py: Page load time benchmark test for proxy in SSL interception mode.
