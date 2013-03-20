#!/usr/bin/env lua
local socket = require "socket"

function resolver_new(opts)
  if not opts then
    return nil, "no options table specified"
  end
  local servers = opts.nameservers
  if not servers or #servers == 0 then
    return nil, "no nameservers specified"
  end
  local timeou = opts.timeout or 2000 -- default 2 secs
  local n = #servers
  local socks = {}
  for i = 1, n do
    local server = servers[i]
    local sock, err = socket.udp()
    if not sock then
      return nil, "failed to create udp socket: " .. err
    end

    local host, port
    if type(server) == 'table' then
      host = server[1]
      port = server[2] or 53
    else
      host = server
      port = 53
      servers[i] = {host, port}
    end

    local ok, err = sock:setpeername(host, port)
--    if not ok then
--      return nil, "failed to set peer name: " .. err
--    end
--    sock:settimeout(timeout)
--    insert(socks, sock)
  end
end

local r, err = resolver_new{
    nameservers = {"8.8.8.8", {"8.8.4.4", 53} },
    retrans = 5,  -- 5 retransmissions on receive timeout
    timeout = 2000,  -- 2 sec
}

--if not r then
--    ngx.say("failed to instantiate the resolver: ", err)
--    return
--end
--
--local answers, err = r:query("www.google.com")
--if not answers then
--    ngx.say("failed to query the DNS server: ", err)
--    return
--end
--
--for i = 1, #answers do
--    local ans = answers[i]
--    ngx.say(ans.name, " ", ans.address or ans.cname,
--            " type:", ans.type, " class:", ans.class,
--            " ttl:", ans.ttl)
--end
