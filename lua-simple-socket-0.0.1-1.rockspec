package = "lua-simple-socket"
version = "0.0.1-1"
source = {
   url = "http://..." -- We don't have one yet
}
description = {
   summary = "A simple socket module for lua.",
   detailed = [[
       It supports: Lua 5.2.
   ]],
   homepage = "https://github.com/Cofyc/lua-simple-socket", -- We don't have one yet
   license = "GPL v2" -- or whatever you like
}
dependencies = {
   "lua ~> 5.2"
   -- If you depend on other rocks, add them here
}
build = {
    type = "make"
}
