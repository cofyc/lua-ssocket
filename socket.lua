local socket = require "socket.c"

socket.tcp = function()
  return socket.socket(socket.AF_INET, socket.SOCK_STREAM)
end

socket.udp = function()
  return socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
end

return socket
