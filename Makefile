PREFIX = /usr/local
RM = rm -f
INSTALL = install -p
INSTALL_EXEC = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644
LUA_VERSION = 5.2
CFLAGS = -Wall -O2 -g -std=c99 -pedantic
MODULE_NAME = simple_socket

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S), Darwin)
	SHARELIB_FLAGS = -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
else
	SHARELIB_FLAGS = -fPIC --shared
endif

all: $(MODULE_NAME).so

LIB_H += $(wildcard *.h)

LIB_OBJS += socket.o
LIB_OBJS += timeout.o
LIB_OBJS += buffer.o

$(LIB_OBJS): $(LIB_H)

$(MODULE_NAME).so: $(LIB_OBJS)
	$(CC) $(CFLAGS) $(SHARELIB_FLAGS) -o $@ $^

install: all
	$(INSTALL_DATA) $(MODULE_NAME).so $(PREFIX)/lib/lua/$(LUA_VERSION)/$(MODULE_NAME).so

uninstall:
	$(RM) $(PREFIX)/lib/lua/$(LUA_VERSION)/$(MODULE_NAME).so

clean:
	$(RM) $(MODULE_NAME).so
	$(RM) -r $(MODULE_NAME).so.dSYM
	$(RM) $(LIB_OBJS)

test: all
	@prove --exec=lua --timer t/test-*.lua

tags:
	find . \( -name .git -type d -prune \) -o \( -name '*.[hc]' -type f -print \) | xargs ctags -a

.PHONY: all install uninstall clean test tags
