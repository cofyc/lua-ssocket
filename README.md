Lua 5.2 socket module
=====================

*I wrote this only for learning purpose. It's not production ready, use http://w3.impa.br/~diego/software/luasocket/ instead.*

Examples
========

    sock = socket.tcp()
    local ok, err = sock:connect("www.google.com", 80)
    if err then
        print(err)
    end
    sock:write("GET / HTTP/1.1\r\n")
    sock:write("\r\n")
    sock:settimeout(1)
    io.output():write(sock:read(1024))
    sock:close()

More examples, see *examples/* folder.

Docs
====

Socket Module
-------------

* socket.socket

    `sock, err = socket.socket(family, type[, protocol])`

* socket.getaddrinfo

    `addrinfos, err = socket.getaddrinfo(host, port[, family=0[, socktype=0[, protocol=0[, flags=0]]]])`

* socket.tcp

    `sock, err = socket.tcp()`

* socket.udp

    `sock, err = socket.udp()`


Socket Object
-------------

* sock:connect

    `ok, err = sock:connect(host, port)`
    `ok, err = sock:connect(unix_socket_path)`

* sock:bind

    `ok, err = sock:bind(host, port)`
    `ok, err = sock:bind(unix_socket_path)`

* sock:listen

    `ok, err = sock:listen(backlog)`

* sock:accept

    `sock, err = sock:accept()`

* sock:write

    `bytes, err = sock:write(data)`

* sock:read

    `data, err = sock:read(maxsize)`

* sock:close

    `sock:close()`

* sock:shutdown

    `ok, err = sock:shutdown(how)`

* sock:fileno

    `fd = sock:fileno()`

* sock:setoption

    `ok, err = sock:setoption(level, optname, value)`

* sock:getoption

    `value, err = sock:getoption(level, optname)`

* sock:settimeout

    `sock:settimeout(timeout)`

* sock:gettimeout

    `timeout = sock:gettimeout()`

* sock:setblocking
    
    `sock:setblocking(block)`

* sock:getpeername

    `addr, err = sock:getpeername()`

* sock:getsockname

    `addr, err = sock:getsockname()`

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
