#!/usr/bin/env lua
require 'Test.More'
require 'Test.Builder.Tester'
local socket = require "socket"

plan(1)

sock = socket.tcp()
local ok, err = sock:connect("www.google.com", 80)
if not ok then
  print(err)
end
local bytes, err = sock:send("GET /notfound HTTP/1.1\r\n")
is(bytes, 24)
local bytes, err = sock:send("\r\n")
is(bytes, 2)
sock:settimeout(0.1)
local ok, err = sock:recv(22)
if not ok then
  is(err, "timed out")
else
  is(sock:recv(22), "HTTP/1.1 404 Not Found")
end
sock:close()
