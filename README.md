Lua 5.2 socket module
=====================

*I wrote this only for learning purpose. It's not production ready, use http://w3.impa.br/~diego/software/luasocket/ instead.*

Examples
========

    socket = require "socket"
    tcpsock = socket.tcp()
    ok, err = tcpsock:connect("www.google.com", 80)
    if err then
        print(err)
    end
    tcpsock:write("GET / HTTP/1.1\r\n")
    tcpsock:write("\r\n")
    tcpsock:settimeout(1)
    data, err, partial = tcpsock:read(1024)
    if err == socket.ERROR_TIMEOUT then
      data = partial
    end
    io.output():write(data)
    tcpsock:close()

More examples, see *examples/* folder.

Docs
====

Socket Module
-------------

* socket.tcp

    `tcpsock, err = socket.tcp()`

* socket.udp

    `udpsock, err = socket.udp()`

* socket.getaddrinfo

    `addrinfos, err = socket.getaddrinfo(host, port[, family=0[, socktype=0[, protocol=0[, flags=0]]]])`

* socket.select

    `readfds, writefds, err = socket.select(readfds, writefds[, timeout=-1])`

TCP Socket Object
-----------------

* tcpsock:connect

    `ok, err = tcpsock:connect(host, port)`
    `ok, err = tcpsock:connect("unix:/path/to/unix-domain.sock")`

* tcpsock:bind

    `ok, err = tcpsock:bind(host, port)`
    `ok, err = tcpsock:bind("unix:/path/to/unix-domain.sock")`

* tcpsock:listen

    `ok, err = tcpsock:listen(backlog)`

* tcpsock:accept

    `tcpsock, err = tcpsock:accept()`

* tcpsock:write

    `bytes, err = tcpsock:write(data)`

* tcpsock:read

    `data, err, partial = tcpsock:read(size)`

    Read specified size of data from socket. This method will not return untile
    it reads exactly the size of data or an error occurs.

    In case of success, it returns the data received; in case of error, it
    returns nil with a string describing the error and the partial data received
    so far.

* tcpsock:readuntil

    `iterator, err = tcpsock:readuntil(pattern, inclusive?)`

    This method returns an iterator function that can be called to read the data
    stream until it sees the specified pattern or an error occurs.

    It also takes an optional inclusive argument to control whether to include
    the pattern string in the returned data string. Default to false.

    For example:

        local reader = tcpsock:readuntil("\r\n")
        while true do
            local data, err, partial = reader()
            if data then
                printf("line: " .. data)
            end
        end

    This iterator function returns the received data right before the specified
    pattern string in the incoming data stream.

    In case of error, it will return nil along with a string describing the
    error and the partial data bytes that have been read so far.

* tcpsock:close
  
    `ok, err = sock:close()`

  Closes the current TCP or stream unix domain socket. It returns the 1 in case
  of success and returns nil with a string describing the error otherwise.

* tcpsock:shutdown

    `ok, err = tcpsock:shutdown(how)`

* tcpsock:fileno

    `fd = tcpsock:fileno()`

* tcpsock:setoption

    `ok, err = tcpsock:setoption(level, optname, value)`

* tcpsock:getoption

    `value, err = tcpsock:getoption(level, optname)`

* tcpsock:settimeout

    `tcpsock:settimeout(timeout)`

* tcpsock:gettimeout

    `timeout = tcpsock:gettimeout()`

* tcpsock:setblocking

    `tcpsock:setblocking(block)`

* tcpsock:getpeername

    `addr, err = tcpsock:getpeername()`

* tcpsock:getsockname

    `addr, err = tcpsock:getsockname()`

UDP Socket Object
-----------------

Contants
--------

SHUT_* sock:shutdown() parameters

 * socket.SHUT_RD
 * socket.SHUT_WR
 * socket.SHUT_RDWR

ERROR_* some error strings, which can be used to detect errors

  * socket.ERROR_TIMEOUT
  * scoekt.ERROR_CLOSED
  * scoekt.ERROR_REFUSED

References
==========

1. http://w3.impa.br/~diego/software/luasocket/reference.html
2. http://golang.org/pkg/net/
3. http://docs.python.org/py3k/library/socket.html
4. http://www.lua.org/manual/5.2/
5. https://github.com/fperrad/lua-TestMore.git
