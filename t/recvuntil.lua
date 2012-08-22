#!/usr/bin/env lua
m = require 'Test.More'
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
local reader, err = sock:recvuntil("\r\n")
if err then
  print(err)
end
while true do
  local data, err = reader()
end
