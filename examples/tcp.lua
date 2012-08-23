#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(4)
sock = socket.tcp()
local ok, err = sock:connect("yechengfu.com", 80)
if err then
  print(err)
end

local addr, err = sock:getpeername()
if err then
  print(err)
end
print(table.unpack(addr))

local addr, err = sock:getsockname()
if err then
  print(err)
end
print(table.unpack(addr))

local bytes, err = sock:send("GET /notfound HTTP/1.1\r\n")
is(bytes, 24)
local bytes, err = sock:send("\r\n")
is(bytes, 2)
timeout = 0.1
sock:settimeout(timeout)
is(sock:gettimeout(), timeout)
--local ok, err = sock:shutdown(socket.SHUT_WR)
local ok, err = sock:recv(22)
if err then
  is(err, "timed out")
else
  is(ok, "HTTP/1.1 404 Not Found")
end
sock:close()
