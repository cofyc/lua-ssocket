#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(1)

sock = socket.tcp()
local ok, err = sock:connect("www.verycd.com", 80)
if not ok then
  print(err)
end

sock:send("GET / HTTP/1.1\r\n")
sock:send("Host: www.verycd.com\r\n")
sock:send("\r\n")
--sock:setblocking(0)
--sock:settimeout(0.1)
--sock:settimeout(1.1)
local data, err = sock:recv(819600)
if not data then
  print(err)
else
  print(#data)
end
