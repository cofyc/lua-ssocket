#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(1)

sock = socket.tcp()
sock:settimeout(1)
local ok, err = sock:connect("yechengfu.com", 80)
if err then
  fail("connect err: " .. err)
end

sock:send("GET / HTTP/1.1\r\n")
sock:send("Host: yechengfu.com\r\n")
sock:send("\r\n")
sock:settimeout(-1)
local data, err = sock:recv(819600)
if err then
  print(err)
  print(#data)
else
  print(#data)
end
