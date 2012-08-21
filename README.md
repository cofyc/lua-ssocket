Lua 5.2 socket module
=====================

!!! This is my first lua c module. I wrote this only for learning purpose. It's not production ready, use http://w3.impa.br/~diego/software/luasocket/ instead.

This module provide access to the BSD socket interface.

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

Unit Test
=========
    make test # requires lua-TestMore (https://github.com/fperrad/lua-TestMore)
