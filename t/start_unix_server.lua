#!/usr/bin/env lua
-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = string.format(";%s/?.lua;%s/../?.lua;", filedir, filedir) .. package.path
package.cpath = string.format(";%s/?.so;%s/../?.so;", filedir, filedir) .. package.cpath

local socket = require "simple_socket"

TEST_UNIX_SOCK = "/tmp/test-socket.sock"

os.remove(TEST_UNIX_SOCK)
local tcpsock = socket.tcp()
local ok, err = tcpsock:bind(TEST_UNIX_SOCK)
tcpsock:listen(5)
conn, err = tcpsock:accept()
os.remove(TEST_UNIX_SOCK)
