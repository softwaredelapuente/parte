CC = gcc
CFLAGS = -Wall -g $(shell pkg-config --cflags x11 xrandr xrender xft)
LIBS = $(shell pkg-config --libs x11 xrandr xrender xft)
SRC = parte.c
OBJ = $(SRC:.c=.o)
PROGRAM = parte
MANPAGE = parte.1

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(PROGRAM)
	mkdir -p $(BINDIR)
	mkdir -p $(MANDIR)/man1
	install -m 755 $(PROGRAM) $(BINDIR)/
	install -m 644 $(MANPAGE) $(MANDIR)/man1/

clean:
	rm -f $(OBJ) $(PROGRAM)

config.h:
	cp config.def.h config.h

uninstall:
	rm -f $(BINDIR)/$(PROGRAM)
	rm -f $(MANDIR)/man1/$(MANPAGE)
