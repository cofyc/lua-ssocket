#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(2)

sock = socket.tcp()
sock:settimeout(1)
local ok, err = sock:connect("yechengfu.com", 80)
if err then
  fail("connect err: " .. err)
end

sock:send("GET / HTTP/1.1\r\n")
sock:send("Host: yechengfu.com\r\n")
sock:send("\r\n")
sock:settimeout(3)  -- enough time
local data, err = sock:recv(10)
is(#data, 10)
is(err, nil)
