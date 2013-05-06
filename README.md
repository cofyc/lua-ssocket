Lua simple socket module
=====================

A simple socket module for lua. It supports: Lua 5.2.

Examples
========

    socket = require "simple_socket"
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

Installation
============

    $ git clone git://github.com/Cofyc/lua-simple-socket.git
    $ make install

By [luarocks](http://luarocks.org/)

    $ luarocks make
    
Docs
====

Socket Module
-------------

* socket.tcp

    `tcpsock, err = socket.tcp()`

* socket.udp

    `udpsock, err = socket.udp()`

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
  
    `ok, err = tcpsock:close()`

  Closes the current TCP or stream unix domain socket. It returns the 1 in case
  of success and returns nil with a string describing the error otherwise.

* tcpsock:shutdown

    `ok, err = tcpsock:shutdown(how)`

* tcpsock:fileno

    `fd = tcpsock:fileno()`

* tcpsock:setopt

    `ok, err = tcpsock:setopt(opt, value)`

* tcpsock:getopt

    `value, err = tcpsock:getopt(level, opt)`

* tcpsock:settimeout

    `tcpsock:settimeout(timeout)`

  Set the timeout in seconds for subsequent socket operations.
  A negative timeout indicates that timeout is disabled, which is default.

* tcpsock:gettimeout

    `timeout = tcpsock:gettimeout()`

  Returns the timeout in seconds associated with socket.
  A negative timeout indicates that timeout is disabled, which is default.

* tcpsock:getpeername

    `addr, err = tcpsock:getpeername()`

* tcpsock:getsockname

    `addr, err = tcpsock:getsockname()`

UDP Socket Object
-----------------

* udpsock:connect

    `ok, err = udpsock:connect(host, port)`
    `ok, err = udpsock:connect("unix:/path/to/unix-domain.sock")`

    Calling connect() on a datagram socket causes the kernel to record a particular
    address as this socket’s peer.
    We can change the peer of a connected datagram socket by issuing a further
    connect() call. 

* udpsock:bind

    `ok, err = udpsock:bind(host, port)`
    `ok, err = udpsock:bind("unix:/path/to/unix-domain.sock")`

* udpsock:recv
  
    `data, err = udpsock:recv(buffersize)`
 
    Receive up to buffersize bytes from UDP or datagram unix domain socket
    object.
   
    In case of success, it returns the data received; in case of error, it
    returns nil with a string describing the error.

* udpsock:recvfrom

    `data, addr, err = udpsock:recvfrom(buffersize)`

    Works exactly as the udpsock:recv method, except it returns the addr as extra
    return values (and is therefore slightly less efficient) in
    case of success.

* udpsock:send

    `ok, err = tcpsock:write(data)`

    Writes data on the current UDP or datagram unix domain socket object.
   
    In case of success, it returns true. Otherwise, it returns nil and a string
    describing the error.

* udpsock:sendto (TODO)

    `ok, err = udpsock:send(data, host, port)`
    `ok, err = udpsock:send(data, "unix:/path/to/unix-domain.sock")`
   
    Writes data on the current UDP or datagram unix domain socket object to
    specified address.
   
    In case of success, it returns true. Otherwise, it returns nil and a string
    describing the error.

* udpsock:close
  
    `ok, err = udpsock:close()`

  Closes the current UDP or datagram unix domain socket. It returns the 1 in
  case of success and returns nil with a string describing the error otherwise.

* udpsock:fileno

    `fd = udpsock:fileno()`

* udpsock:settimeout

    `udpsock:settimeout(timeout)`

* udpsock:gettimeout

    `timeout = udpsock:gettimeout()`

Contants
--------

Module infos:
    
  * socket._VERSION

OPT_* are setopt and getopt parameters:

  * socket.OPT_TCP_NODELAY
  * socket.OPT_TCP_KEEPALIVE
  * socket.OPT_TCP_REUSEADDR

SHUT_* are tcpsock:shutdown() parameters:

  * socket.SHUT_RD
  * socket.SHUT_WR
  * socket.SHUT_RDWR

ERROR_* are predefined error strings, which can be used to detect errors:

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
