local socket = require "socket.c"

socket.tcp = function()
  return socket.socket(socket.AF_INET, socket.SOCK_STREAM)
end

return socket
