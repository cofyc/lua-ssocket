PREFIX = /usr/local
RM = rm -f
INSTALL = install -p
INSTALL_EXEC = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644
LUA_VERSION = 5.2
CFLAGS = -Wall -O2 -g -std=c99 -pedantic

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S), Darwin)
	SHARELIB_FLAGS = -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
else
	SHARELIB_FLAGS = -fPIC --shared
endif

all: socket.so

LIB_H += $(wildcard *.h)

LIB_OBJS += socket.o
LIB_OBJS += timeout.o
LIB_OBJS += buffer.o

$(LIB_OBJS): $(LIB_H)

socket.so: $(LIB_OBJS)
	$(CC) $(CFLAGS) $(SHARELIB_FLAGS) -o socket.so $^

install: all
	$(INSTALL_DATA) socket.so $(PREFIX)/lib/lua/$(LUA_VERSION)

uninstall:
	$(RM) $(PREFIX)/lib/lua/$(LUA_VERSION)/socket.so

clean:
	$(RM) socket.so
	$(RM) -r socket.so.dSYM
	$(RM) $(LIB_OBJS)

test: all
	@prove --exec=lua --timer t/*.lua

tags:
	find . \( -name .git -type d -prune \) -o \( -name '*.[hc]' -type f -print \) | xargs ctags -a

.PHONY: all install uninstall clean test tags
