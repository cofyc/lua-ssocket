-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = package.path .. string.format(";%s/?.lua;%s/../?.lua", filedir, filedir)
package.cpath = package.cpath .. string.format(";%s/?.so;%s/../?.so", filedir, filedir)

require 'Test.More'
local socket = require "socket"

-- 1. Error: socket.ERROR_REFUSED/socket.ERROR_CLOSED
local tcpsock = socket.tcp()
local ok, err = tcpsock:connect("unix:/tmp/nosuchfile.sock")
is(ok, nil)
