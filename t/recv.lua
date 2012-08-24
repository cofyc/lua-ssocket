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

sock:write("GET / HTTP/1.1\r\n")
sock:write("Host: yechengfu.com\r\n")
sock:write("\r\n")
sock:settimeout(3)  -- enough time
local data, err = sock:read(10)
is(#data, 10)
is(err, nil)
