BASIC_CFLAGS = -Wall -O3 -fPIC -g -std=c99 -pedantic

ALL_CFLAGS = $(BASIC_CFLAGS) $(CFLAGS)

PREFIX = /usr/local
RM = rm -f
INSTALL = install -p
INSTALL_EXEC = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644
LUA_VERSION = 5.2
MODULE_NAME = ssocket

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S), Darwin)
	SHARELIB_FLAGS = -dynamiclib -Wl,-undefined,dynamic_lookup
else
	SHARELIB_FLAGS = --shared
endif

all: $(MODULE_NAME).so

LIB_H += $(wildcard *.h)

OBJECTS += socket.o
OBJECTS += timeout.o
OBJECTS += buffer.o

$(OBJECTS): $(LIB_H)

$(OBJECTS): %.o: %.c
	$(CC) -o $*.o -c $(ALL_CFLAGS) $<

$(MODULE_NAME).so: $(OBJECTS)
	$(CC) $(SHARELIB_FLAGS) -o $@ $^

install: all
	$(INSTALL_DATA) $(MODULE_NAME).so $(PREFIX)/lib/lua/$(LUA_VERSION)/$(MODULE_NAME).so

uninstall:
	$(RM) $(PREFIX)/lib/lua/$(LUA_VERSION)/$(MODULE_NAME).so

test: all
	@prove --exec=lua --timer t/test-*.lua

tags:
	find . \( -name .git -type d -prune \) -o \( -name '*.[hc]' -type f -print \) | xargs ctags -a

clean:
	$(RM) $(MODULE_NAME).so
	$(RM) -r $(MODULE_NAME).so.dSYM
	$(RM) $(OBJECTS)

.PHONY: all install uninstall clean test tags
