#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(2)

function bigdata(len)
  local s = ''
  local t = {}
  for i=1,len do t[i] = 'a' end
  return table.concat(t)
end

sock = socket.tcp()
local ok, err = sock:connect("yechengfu.com", 80)
if err then
  print(err)
end

sock:send("POST / HTTP/1.1\r\n")
sock:send("Host: yechengfu.com\r\n")
is(sock:send("\r\n"), 2)

-- send big data
local data, err = sock:send(bigdata(8192*100))
is(err, "Connection closed")

sock:close()
