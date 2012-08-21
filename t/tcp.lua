#!/usr/bin/env lua
require 'Test.More'
require 'Test.Builder.Tester'
local socket = require "socket"

--local addrinfo, err = socket.getaddrinfo("www.verycd.com", 80);
--if not addrinfo then
--  print(err)
--else
--  for i, addr in ipairs(addrinfo) do
--    print("family:", addr.family)
--    print("socktype:", addr.socktype)
--    print("protocol:", addr.protocol)
--    print("canonname:", addr.canonname)
--    print("addr:", addr.addr)
--  end
--end

plan(1)

sock = socket.tcp()
local ok, err = sock:connect("www.google.com", 80)
sock:send("GET /notfound HTTP/1.1\r\n")
sock:send("\r\n")
sock:settimeout(1)
is(sock:recv(22), "HTTP/1.1 404 Not Found")
sock:close()
