#!/usr/bin/env lua
require 'Test.More'
require 'Test.Builder.Tester'
local socket = require "socket"

plan(1)
local ok, err = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 100)
if not ok then
  is(err, "Protocol not supported")
end
