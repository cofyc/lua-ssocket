#!/usr/bin/env lua
local socket = require "ssocket"

HOST = '127.0.0.1'
PORT = 12345
tcpsock = socket.tcp()
local ok, err = tcpsock:bind(HOST, PORT)
if err then
  print(err)
  os.exit()
end
tcpsock:listen(5)

addr, err = tcpsock:getsockname()
print(string.format("Listening on %s:%d...", addr[1], addr[2]))
print(string.format("You can use this command to connect on: telnet %s %d", addr[1], addr[2]))
print("")

while true do
  conn, err = tcpsock:accept()
  addr, err = conn:getpeername()
  print(string.format("[%d] Connected from %s:%d.", conn:fileno(), addr[1], addr[2]))
  if err then
    print(err)
    os.exit()
  else
    reader, err = conn:readuntil("\n")
    if err then
      print(err)
    else
      while true do
        local data, err, partial = reader()
        if err then
          if err == socket.ERROR_CLOSED then
            print(string.format("[%d] Connection closed, exit.", conn:fileno()))
          end
          break
        end
        if partial then
          data = partial
        end
        print(string.format("[%d] %s", conn:fileno(), data))
      end
    end
  end
end
