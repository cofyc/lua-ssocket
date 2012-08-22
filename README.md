Lua 5.2 socket module
=====================

*This is my first lua c module. I wrote this only for learning purpose. It's not production ready, use http://w3.impa.br/~diego/software/luasocket/ instead.*

Examples
========

    sock = socket.tcp()
    local ok, err = sock:connect("www.google.com", 80)
    if not ok then
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

* sockobj:connect
* sockobj:send
* sockobj:recv
* sockobj:close
* sockobj:shutdown
* sockobj:fileno
* sockobj:setoption
* sockobj:getoption
* sockobj:settimeout
* sockobj:gettimeout
* sockobj:setblocking

Contants
--------

References
==========

1. http://w3.impa.br/~diego/software/luasocket/reference.html
2. http://golang.org/pkg/net/
3. http://docs.python.org/py3k/library/socket.html
4. http://www.lua.org/manual/5.2/
