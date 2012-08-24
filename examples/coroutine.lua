#!/usr/bin/env lua
local socket = require "socket"

paths = {
  '/archives/game/00001.html',
  '/archives/game/00002.html',
  '/archives/game/00003.html',
  '/archives/game/00004.html',
  '/archives/game/00005.html',
  '/archives/game/00006.html',
  '/archives/game/00007.html',
  '/archives/game/00008.html',
}

contents = {
}

function worker(path)
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
    local data, err = sock:read(8192)
    if contents[path] == nil then
      contents[path] = data
    else
      contents[path] = contents[path] .. data
    end
    if err then
      if err == socket.ERROR_CLOSED then
        sock:close()
      end
      break
    end
    coroutine.yield(#data)
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
        local data, err = sock:read(8192)
        if contents[path] == nil then
          contents[path] = data
        else
          contents[path] = contents[path] .. data
        end
        if err then
          if err == socket.ERROR_CLOSED then
            sock:close()
          end
          break
        end
      end
    end
else
    local threads = {}

    for i, path in ipairs(paths) do
      local thread = coroutine.create(function () worker(path) end)
      table.insert(threads, thread)
    end

    while true do
      if 0 == #threads then
        -- no threads
        break
      end
      for i, thread in ipairs(threads) do
        local status, result = coroutine.resume(thread)
        if result == nil then
          table.remove(threads, i)
        end
      end
    end
end

for path, content in pairs(contents) do
  print(path, #content)
end
