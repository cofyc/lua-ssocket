-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = string.format(";%s/?.lua;%s/../?.lua;", filedir, filedir) .. package.path
package.cpath = string.format(";%s/?.so;%s/../?.so;", filedir, filedir) .. package.cpath

local socket = require "simple_socket"

local udpsock = socket.udp()
local ok, err = udpsock:bind("127.0.0.1", 8888)
data, err = udpsock:recv(8192)
print(data)
