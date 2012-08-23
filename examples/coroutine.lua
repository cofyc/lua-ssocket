#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(1)

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

function worker(path)
  local sock = socket.tcp()
  local ok, err = sock:connect('yechengfu.com', 80)
  if err then
    print(err)
    return
  end
  coroutine.yield()
  sock:send("GET " .. path .. " HTTP/1.1\r\n")
  sock:send("Host: yechengfu.com\r\n")
  sock:send("\r\n")
  local ok, err = sock:recv(1024)
  sock:close()
  print(path, #ok)
end

local threads = {}

for i, path in ipairs(paths) do
  local thread = coroutine.create(function () worker(path) end)
  table.insert(threads, thread)
end

while true do
  -- no more threads ?
  if #threads == 0 then
    break
  end
  local cc = table.remove(threads, 1)
  local ok = coroutine.resume(cc)
  if ok then
    table.insert(threads, cc)
  end
end
