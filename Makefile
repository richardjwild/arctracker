# Copyright (c) Richard Wild 2004, 2005
#
# This file is part of Arctracker.
#
# Arctracker is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Arctracker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Arctracker; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MS 02111-1307 USA

CC=gcc
INSTALL=/usr/bin/install -c
CFLAGS=-g -O2 
LDFLAGS=
CLIBS= 
INSTALLDIR=/usr/local

arctracker: arctracker.o initialise.o read_mod.o play_mod.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLIBS) -o arctracker arctracker.o initialise.o read_mod.o play_mod.o

arctracker.o: arctracker.h arctracker.c
	$(CC) $(CFLAGS) -c arctracker.c

initialise.o: arctracker.h initialise.c
	$(CC) $(CFLAGS) -c initialise.c

read_mod.o: arctracker.h read_mod.c
	$(CC) $(CFLAGS) -c read_mod.c

play_mod.o: arctracker.h log_lin_tab.h play_mod.c
	$(CC) $(CFLAGS) -c play_mod.c

clean:
	rm -f ./arctracker ./*.o 2>/dev/null

install: arctracker
	$(INSTALL) ./arctracker $(INSTALLDIR)/bin/arctracker

uninstall:
	rm -f $(INSTALLDIR)/bin/arctracker 2>/dev/null
