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
like(udpsock, "<udpsock: %d+>") -- __tostring
is(udpsock:close(), true)
