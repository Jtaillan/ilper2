# ILPER 1.35 for Linux
#
# Copyright (c) 2008-2009  J-F Garnier
# Copyright (c) 2011-2012  Ch. Gottheimer
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ----------------------------------------------------------------------------------
#
# Makefile    ILPER make(1)
#
# All runtime configuration (device, LIF_file, baudrate, scope, printer)
# is read from ~/.config/ilper/ilper.conf at startup.
#
# 2011: ported on Linux by Ch. Gottheimer
# 2026: ported to MacOS and enhanced
# ---------------------------------------------------------------------------------

CC	= clang
CFLAGS	= -O2 -Wall -D_XOPEN_SOURCE_EXTENDED
LDFLAGS	=

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS: system curses includes wide-char support
    LIBS = -lcurses
    # ARCH: leave empty for native, or set on command line:
    #   make ARCH=arm64      -> Apple Silicon only
    #   make ARCH=x86_64     -> Intel only
    #   make ARCH=universal  -> fat binary (arm64 + x86_64)
    ifdef ARCH
        ifeq ($(ARCH),universal)
            ARCHFLAGS = -arch arm64 -arch x86_64
        else
            ARCHFLAGS = -arch $(ARCH)
        endif
        CFLAGS  += $(ARCHFLAGS)
        LDFLAGS += $(ARCHFLAGS)
    endif

else ifeq ($(UNAME_S),Linux)
    # Linux: requires explicit -lncursesw for wide-char support
    LIBS = -lncursesw

else
    $(error Unsupported OS: $(UNAME_S))
endif

ILPER    = ilper

SOURCES = ilmain.c ilper7.c ilbase.c ildisp.c ilprint.c ildrive.c scope.c roman8.c
OBJECTS = $(SOURCES:%.c=%.o)

PREFIX  := /usr/local
BINDIR  := $(PREFIX)/bin
MANDIR  := $(PREFIX)/share/man/man1

%.o: %.c ilper.h ilbase.h
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(ILPER)

$(ILPER): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

clean:
	@echo "Cleaning $(ILPER)"
	@\rm -f $(ILPER) $(OBJECTS)

install: $(ILPER)
	install -d $(BINDIR) $(MANDIR)
	install -s -m 755 $(ILPER) $(BINDIR)/$(ILPER)
	install -m 644 $(ILPER).1 $(MANDIR)/$(ILPER).1
