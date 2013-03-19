#!/usr/bin/env lua
local socket = require "socket"

HOST = 'localhost'
PORT = 12345
sock = socket.tcp()
sock:connect(HOST, PORT)
sock:write("Hello, World!")
data = sock:read(1024)
sock:close()
print("Received:", data)
