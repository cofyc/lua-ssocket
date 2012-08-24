#!/usr/bin/env lua
local socket = require "socket"

local addrinfo, err = socket.getaddrinfo("yechengfu.com", 80);
if err then
  print(err)
else
  for i, addr in ipairs(addrinfo) do
    print("======")
    print("family:", addr.family)
    print("socktype:", addr.socktype)
    print("protocol:", addr.protocol)
    print("canonname:", addr.canonname)
    print("addr:", addr.addr)
  end
end
