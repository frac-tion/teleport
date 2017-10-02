CC ?= gcc
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0 avahi-client libsoup-2.4)
LIBS = $(shell $(PKGCONFIG) --libs gtk+-3.0 avahi-client libsoup-2.4)
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

SRC = teleport.c teleportwin.c teleportpeer.c main.c browser.c publish.c server.c get.c
BUILT_SRC = resources.c

OBJS = $(BUILT_SRC:.c=.o) $(SRC:.c=.o)

all: teleport

resources.c: teleport.gresource.xml window.ui settings.ui remote_list.ui
	$(GLIB_COMPILE_RESOURCES) teleport.gresource.xml --target=$@ --sourcedir=. --generate-source

%.o: %.c
	$(CC) -c -o $(@F) $(CFLAGS) $<

teleport: $(OBJS)
	$(CC) -o $(@F) $(OBJS) $(LIBS)

clean:
	rm -f $(BUILT_SRC)
	rm -f $(OBJS)
	rm -f teleport


