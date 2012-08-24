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
local ok, err = sock:connect("google.com", 80)
if err then
  print(err)
end

sock:write("POST / HTTP/1.1\r\n")
sock:write("Host: google.com\r\n")
is(sock:write("\r\n"), 2)

-- send big data
local data, err = sock:write(bigdata(8192*100))
type_ok(err, "string")


sock:close()
