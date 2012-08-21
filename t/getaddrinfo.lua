#!/usr/bin/env lua
require 'Test.More'
require 'Test.Builder.Tester'
local socket = require "socket"

local addrinfo, err = socket.getaddrinfo("www.verycd.com", 80);
if not addrinfo then
  print(err)
else
  for i, addr in ipairs(addrinfo) do
    print("family:", addr.family)
    print("socktype:", addr.socktype)
    print("protocol:", addr.protocol)
    print("canonname:", addr.canonname)
    print("addr:", addr.addr)
  end
end
