#!/usr/bin/env lua
local socket = require "ssocket"

SOCK_PATH = '/tmp/test.sock'
tcpsock = socket.tcp()
os.remove(SOCK_PATH)
local ok, err = tcpsock:bind(SOCK_PATH)
if err then
  print(err)
  os.exit()
end
tcpsock:listen(5)

addr, err = tcpsock:getsockname()
print(string.format("Listening on %s...", addr))
print(string.format("You can use this command to connect on: socat - UNIX-CONNECT:%s", SOCK_PATH))
print("")
while true do
  conn, err = tcpsock:accept()
  addr, err = conn:getpeername()
  if type(addr) == "string" then
    print(string.format("[%d] Connected from %s.", conn:fileno(), addr))
  else
    print(string.format("[%d] Connected from %s:%d.", conn:fileno(), addr[1], addr[2]))
  end
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
