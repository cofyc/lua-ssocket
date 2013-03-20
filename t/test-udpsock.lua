#!/usr/bin/env lua
-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = package.path .. string.format(";%s/?.lua;%s/../?.lua", filedir, filedir)
package.cpath = package.cpath .. string.format(";%s/?.so;%s/../?.so", filedir, filedir)

require 'Test.More'
local socket = require "socket"

plan(16)

-- 1. Basic
local udpsock = socket.udp()
ok, err = udpsock:connect('127.0.0.1', 8888)
is(ok, true)
is(err, nil)
like(udpsock, "<udpsock: %d+>") -- __tostring

udpsock:send("OK")
is(udpsock:close(), true)
