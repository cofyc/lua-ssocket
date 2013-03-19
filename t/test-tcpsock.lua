#!/usr/bin/env lua
-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = package.path .. string.format(";%s/?.lua;%s/../?.lua", filedir, filedir)
package.cpath = package.cpath .. string.format(";%s/?.so;%s/../?.so", filedir, filedir)

require 'Test.More'
local socket = require "socket"

plan(15)

function bigdata(len)
  local s = ''
  local t = {}
  for i=1,len do t[i] = 'a' end
  return table.concat(t)
end

-- 1. Success connection.

local tcpsock = socket.tcp()
local ok, err = tcpsock:connect("yechengfu.com", 80)
is(ok, true)
is(err, nil)
tcpsock:close()

-- 2. Errors
is(socket.ERROR_TIMEOUT, "Operation timed out")
is(socket.ERROR_CLOSED, "Connection closed")
is(socket.ERROR_REFUSED, "Connection refused")

-- 3. Error: socket.ERROR_REFUSED/socket.ERROR_CLOSED
local tcpsock = socket.tcp()
local ok, err = tcpsock:connect("127.0.0.1", 16787)
is(ok, nil)
is(err, socket.ERROR_REFUSED)
local bytes, err = tcpsock:write("hello")
is(bytes, nil)
is(err, socket.ERROR_CLOSED)
local data, err, partial = tcpsock:read(1024)
is(err, socket.ERROR_CLOSED)
tcpsock:close()

-- 4. Error: socket.ERROR_TIMEOUT
local tcpsock = socket.tcp()
tcpsock:settimeout(0.001)
local ok, err = tcpsock:connect("yechengfu.com", 16787)
is(ok, nil)
is(err, socket.ERROR_TIMEOUT)
local bytes, err = tcpsock:write("hello")
is(bytes, nil)
is(err, socket.ERROR_CLOSED)
local data, err, partial = tcpsock:read(1024)
is(err, socket.ERROR_CLOSED)
tcpsock:close()

--
--tcpsock = socket.tcp()
--tcpsock:settimeout(0.001) -- impossible to connect remote address in such small time
--local ok, err = tcpsock:connect("yechengfu.com", 80)
--is(ok, nil)
--is(err, socket.ERROR_TIMEOUT)
--
--tcpsock = socket.tcp()
--tcpsock:settimeout(3) -- big enough time to connect this remote address
--local ok, err = tcpsock:connect("yechengfu.com", 1234)
--is(ok, nil)
--is(err, socket.ERROR_TIMEOUT)
--
--tcpsock = socket.tcp()
--local ok, err = tcpsock:connect("localhost", 1234)
--is(ok, nil)
--tcpsock = socket.tcp()
--local ok, err = tcpsock:connect("google.com", 80)
--if err then
--  print(err)
--end
--
--tcpsock:write("POST / HTTP/1.1\r\n")
--tcpsock:write("Host: google.com\r\n")
--is(tcpsock:write("\r\n"), 2)
--
---- send big data
--local data, err = tcpsock:write(bigdata(8192*100))
--type_ok(err, "string")
