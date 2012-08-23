#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(1)

sock = socket.tcp()
local ok, err = sock:connect("yechengfu.com", 80)
if err then
  print(err)
end

sock:send("GET / HTTP/1.1\r\n")
sock:send("Host: upload.verycd.com\r\n")
sock:send("\r\n")
--sock:setblocking(0)
--sock:settimeout(1.101)
--sock:settimeout(1.1)
local s = ''
local t = {}
for i=1,8292*100 do t[i] = 'a' end
s = table.concat(t)
local data, err = sock:send(s)
if err then
  print(err)
else
  is(#s, data)
end
