require 'Test.More'
local socket = require "socket"

plan(3)

sock = socket.tcp()
local ok, err = sock:connect("www.baidu.com", 80)
if err then
  fail("connect err: " .. err)
end

sock:write("GET /robots.txt HTTP/1.1\r\n")
sock:write("Host: www.baidu.com\r\n")
sock:write("\r\n")
sock:settimeout(3)  -- enough time
local data, err, partial = sock:read(4)
is(data, "HTTP")
is(err, nil)
is(partial, nil)
