#!/usr/bin/env lua
m = require 'Test.More'
local socket = require "socket"

plan(1)
function foo() 
  sock = socket.tcp()
end
foo()
collectgarbage()
