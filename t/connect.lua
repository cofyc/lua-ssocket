#!/usr/bin/env lua
require 'Test.More'
local socket = require "socket"

plan(12)

sock = socket.tcp()

local ok, err = sock:connect("www.google.com", 80)
is(ok, true)
is(err, nil)

local ok, err = sock:connect("www.google.com", 81)
is(ok, nil)
is(err, "Socket is already connected")

sock = socket.tcp()
sock:settimeout(0)
local ok, err = sock:connect("www.google.com", 80)
is(ok, nil)
is(err, "Operation now in progress")

sock = socket.tcp()
sock:settimeout(0.001) -- impossible to connect remote address in such small time
local ok, err = sock:connect("www.google.com", 80)
is(ok, nil)
is(err, socket.ERROR_TIMEOUT)

sock = socket.tcp()
sock:settimeout(3) -- big enough time to connect this remote address
local ok, err = sock:connect("www.google.com", 1234)
is(ok, nil)
is(err, socket.ERROR_TIMEOUT)

sock = socket.tcp()
local ok, err = sock:connect("localhost", 1234)
is(ok, nil)
is(err, "Connection refused")
