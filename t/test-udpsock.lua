-- setup path
local filepath = debug.getinfo(1).source:match("@(.*)$")
local filedir = filepath:match('(.+)/[^/]*') or '.'
package.path = string.format(";%s/?.lua;%s/../?.lua;", filedir, filedir) .. package.path
package.cpath = string.format(";%s/?.so;%s/../?.so;", filedir, filedir) .. package.cpath

require 'Test.More'
local socket = require "ssocket"

plan(12)

function string_repeat(str, num)
  local s = ""
  for i = 1, num do
    s = s .. str
  end
  return s
end
local packet_data = "A udp packet."

-- 1. Basic
local udpsock = socket.udp()
ok, err = udpsock:connect('localhost', 8888)
is(ok, true)
like(udpsock, "<udpsock: %d+>") -- __tostring
is(udpsock:send("OK"), true)
is(udpsock:close(), true)
udpsock:close()

-- 2. Exchange datagram packet
local recvsock = socket.udp()
ok, err = recvsock:bind('localhost', 8888)
is(ok, true)
local sendsock = socket.udp()
sendsock:connect('localhost', 8888)
is(sendsock:send(packet_data), true)
data, err = recvsock:recv(8192)
is(data, packet_data)

-- 3. sendto/recvfrom
local sendsock = socket.udp()
ok, err = sendsock:sendto(packet_data, 'localhost', 8888)
data, addr, err = recvsock:recvfrom(8192)
is(data, packet_data)
is(addr[1], '127.0.0.1')
type_ok(addr[2], "number")

-- 4. send big data
longstr = string_repeat("a",165507)
ok, err = sendsock:sendto(longstr, "8.8.8.8", 53)
is(ok, nil)
is(err, "Message too long")
