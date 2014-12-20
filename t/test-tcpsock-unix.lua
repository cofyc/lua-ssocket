-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = string.format(";%s/?.lua;%s/../?.lua;", filedir, filedir) .. package.path
package.cpath = string.format(";%s/?.so;%s/../?.so;", filedir, filedir) .. package.cpath

require 'Test.More'
local socket = require "ssocket"
local lua_bin = os.getenv("LUA") or "lua"

plan(6)

TEST_UNIX_SOCK = "/tmp/test-socket.sock"

-- 1. Error
local tcpsock = socket.tcp()
local ok, err = tcpsock:connect("unix:/tmp/nosuchfile.sock")
is(ok, nil)
is(err, "No such file or directory")
tcpsock:close()

-- 2. bind
os.remove(TEST_UNIX_SOCK)
local tcpsock = socket.tcp()
local ok, err = tcpsock:bind(TEST_UNIX_SOCK)
is(ok, true)
is(err, nil)
tcpsock:close()

-- 3. connect
os.execute(string.format("%s %s/start_unix_server.lua &>/dev/null &", lua_bin, filedir))
os.execute("sleep 1") -- make sure service is on
local tcpsock = socket.tcp()
local ok, err = tcpsock:connect(TEST_UNIX_SOCK)
is(ok, true)
is(err, nil)
