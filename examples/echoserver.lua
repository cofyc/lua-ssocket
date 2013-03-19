#!/usr/bin/env lua
local socket = require "socket"

HOST = '0.0.0.0'
PORT = 12345
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
local ok, err = sock:bind(HOST, PORT)
if err then
  print(err)
end
sock:listen(5)

conn, err = sock:accept()
if err then
  print(err)
else 
  while true do
    data, err = conn:read(1024)
    if err then break end
    conn:write(data)
  end
end
