#!/usr/bin/env lua
-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = string.format(";%s/?.lua;%s/../?.lua", filedir, filedir) .. package.path 
package.cpath = string.format(";%s/?.so;%s/../?.so", filedir, filedir) .. package.cpath 

require 'Test.More'
local socket = require "socket"

plan(5)

-- 1. Basic
local udpsock = socket.udp()
ok, err = udpsock:connect('localhost', 8888)
is(ok, true)
is(err, nil)
like(udpsock, "<udpsock: %d+>") -- __tostring
is(udpsock:write("OK"), 2)
is(udpsock:close(), true)
