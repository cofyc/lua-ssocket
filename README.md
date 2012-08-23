Lua 5.2 socket module
=====================

*I wrote this only for learning purpose. It's not production ready, use http://w3.impa.br/~diego/software/luasocket/ instead.*

Examples
========

More examples, see *examples/* folder.

    sock = socket.tcp()
    local ok, err = sock:connect("www.google.com", 80)
    if err then
        print(err)
    end
    sock:send("GET / HTTP/1.1\r\n")
    sock:send("\r\n")
    sock:settimeout(1)
    io.output():write(sock:recv(1024))
    sock:close()

Docs
====
    
Socket Module
-------------

* socket.socket
* socket.getaddrinfo
* socket.tcp
* socket.udp


Socket Object
-------------

* sock:connect
* sock:send
* sock:recv
* sock:close
* sock:shutdown
* sock:fileno
* sock:setoption
* sock:getoption
* sock:settimeout
* sock:gettimeout
* sock:setblocking
* sock:getpeername
* sock:getsockname

Contants
--------

These constants represent the address (and protocol) families, used for the first argument to socket().

* socket.AF_INET
* socket.AF_INET6
* socket.AF_UNIX

These constants represent the socket types, used for the second argument to socket(). (Only SOCK_STREAM and SOCK_DGRAM appear to be generally useful.)

* socket.SOCK_STREAM
* socket.SOCK_DGRAM
* socket.SOCK_RAW
* socket.SOCK_RDM
* socket.SOCK_SEQPACKET

AI_*

 * socket.AI_PASSIVE
 * socket.AI_CANONNAME
 * socket.AI_NUMERICHOST
 * socket.AI_NUMERICSERV
 * socket.AI_MASK
 * socket.AI_ALL
 * socket.AI_V4MAPPED_CFG
 * socket.AI_ADDRCONFIG
 * socket.AI_V4MAPPED
 * socket.AI_DEFAULT

INADDR_* Some reserved IPv4 addresses

 * socket.INADDR_ANY
 * socket.INADDR_BROADCAST
 * socket.INADDR_LOOPBACK
 * socket.INADDR_UNSPEC_GROUP
 * socket.INADDR_ALLHOSTS_GROUP
 * socket.INADDR_MAX_LOCAL_GROUP
 * socket.INADDR_NONE

TCP_* Tcp options

 * socket.TCP_NODELAY
 * socket.TCP_KEEPALIVE

SHUT_* sock:shutdown() parameters

 * socket.SHUT_RD
 * socket.SHUT_WR
 * socket.SHUT_RDWR

ERROR_* some error strings, which can be used to detect errors
  * socket.ERROR_TIMEOUT
  * scoekt.ERROR_CLOSED

Misc.

* socket.SOMAXCONN

References
==========

1. http://w3.impa.br/~diego/software/luasocket/reference.html
2. http://golang.org/pkg/net/
3. http://docs.python.org/py3k/library/socket.html
4. http://www.lua.org/manual/5.2/
