#!/usr/bin/env lua
local socket = require "ssocket"

local paths = {}
local contents = {}

for i = 1, 20 do
  local path = string.format('/archives/game/%05d.html', i)
  table.insert(paths, path)
end

function worker(sock, path)
  local ok, err = sock:connect('www.verycd.com', 80)
  if err then
    print(path .. " failed")
    return
  end

  sock:write("GET " .. path .. " HTTP/1.1\r\n")
  sock:write("Host: www.verycd.com\r\n")
  sock:write("\r\n")
  coroutine.yield(true)
  while true do
    local data, err, partial = sock:read(8192)
    if err then
      if err == socket.ERROR_CLOSED then
        data = partial
      end
    end
    if contents[path] == nil then
      contents[path] = data
    else
      contents[path] = contents[path] .. data
    end
    if err then
      break
    end
    coroutine.yield(true)
  end
  coroutine.yield(nil)
end

if arg[1] == "sequence" then
  print("sequence...")
  for i, path in ipairs(paths) do
    local sock = socket.tcp()
    local ok, err = sock:connect('www.verycd.com', 80)
    if err then
      print(path .. " failed")
      return
    end

    sock:write("GET " .. path .. " HTTP/1.1\r\n")
    sock:write("Host: www.verycd.com\r\n")
    sock:write("\r\n")

    while true do
      local data, err, partial = sock:read(8192)
      if err then
        if err == socket.ERROR_CLOSED then
          data = partial
        end
      end
      if contents[path] == nil then
        contents[path] = data
      else
        contents[path] = contents[path] .. data
      end
      if err then
        break
      end
    end
  end
else
  print("selecting...")
  local threads = {}

  for i, path in ipairs(paths) do
    local sock = socket.tcp()
    local thread = coroutine.create(function () worker(sock, path) end)
    coroutine.resume(thread) -- initialize
    threads[sock:fileno()] = thread
  end

  while true do
    local fds = {}
    for fd, _ in pairs(threads) do
      table.insert(fds, fd)
    end

    if 0 == #fds then
      -- no more to do
      break
    end

    local readfds, writefds, err = socket.select(fds, fds)

    if err then
      print(err)
      os.exit()
    elseif readfds then
      for _, fd in ipairs(readfds) do
        local status, result = coroutine.resume(threads[fd])
        if result == nil then
          threads[fd] = nil
        end
      end
    end
  end
end

for _, path in ipairs(paths) do
  print(path, #contents[path])
end
