
PREFIX = /usr/local
RM = rm -f
INSTALL = install -p
INSTALL_EXEC = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644
LUA_VERSION = 5.2

all:
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "    macosx linux"

macosx: socket.c timeout.c
	$(CC) -g -Wall -fno-common -dynamiclib -o socket.so $^ -llua

linux: socket.c timeout.c
	$(CC) -g -Wall -fPIC --shared -o socket.so $^ -llua

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
