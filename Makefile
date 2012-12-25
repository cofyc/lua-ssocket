
PREFIX = /usr/local
RM = rm -f
INSTALL = install -p
INSTALL_EXEC = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644
LUA_VERSION = 5.2

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S), Darwin)
	SHARELIB_FLAGS = -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
else
	SHARELIB_FLAGS = -fPIC --shared
endif

all: socket.so

socket.so: socket.c timeout.c
	$(CC) -g -Wall $(SHARELIB_FLAGS) -o socket.so $^

install:
	$(INSTALL_DATA) socket.so $(PREFIX)/lib/lua/$(LUA_VERSION)
	$(INSTALL_DATA) socket.lua $(PREFIX)/lib/lua/$(LUA_VERSION)

uninstall:
	$(RM) $(PREFIX)/lib/lua/$(LUA_VERSION)/socket.so
	$(RM) $(PREFIX)/lib/lua/$(LUA_VERSION)/socket.lua

clean:
	$(RM) socket.so
	$(RM) -r socket.so.dSYM

test:
	@prove --exec=lua --timer t/*.lua

tags:
	find . \( -name .git -type d -prune \) -o \( -name '*.[hc]' -type f -print \) | xargs ctags -a

.PHONY: all install uninstall clean test tags
