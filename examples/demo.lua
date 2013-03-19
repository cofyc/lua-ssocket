#!/usr/bin/env lua
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
