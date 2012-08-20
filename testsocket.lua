local socket = require "socket"

local addrinfo, err = socket.getaddrinfo("www.verycd.com", 80);
if not addrinfo then
  print(err)
else
  for i, addr in ipairs(addrinfo) do
    print("family:", addr.family)
    print("socktype:", addr.socktype)
    print("protocol:", addr.protocol)
    print("canonname:", addr.canonname)
    print("addr:", addr.addr)
  end
end

sock = socket.tcp()
local ok, err = sock:connect("www.verycd.com", 80)
sock:send("GET / HTTP/1.1\r\n")
sock:send("\r\n")
sock:settimeout(1)
io.output():write(sock:recv(1024))
sock:close()
