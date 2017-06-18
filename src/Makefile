CC ?= gcc
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0 avahi-client)
LIBS = $(shell $(PKGCONFIG) --libs gtk+-3.0 avahi-client)
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

SRC = teleportapp.c teleportappwin.c teleportpeer.c main.c browser.c
BUILT_SRC = resources.c

OBJS = $(BUILT_SRC:.c=.o) $(SRC:.c=.o)

all: teleportapp

resources.c: teleportapp.gresource.xml window.ui settings.ui remote_list.ui
	$(GLIB_COMPILE_RESOURCES) teleportapp.gresource.xml --target=$@ --sourcedir=. --generate-source

%.o: %.c
	$(CC) -c -o $(@F) $(CFLAGS) $<

teleportapp: $(OBJS)
	$(CC) -o $(@F) $(OBJS) $(LIBS)

clean:
	rm -f $(BUILT_SRC)
	rm -f $(OBJS)
	rm -f teleportapp


