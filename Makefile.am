CC=@CC@
INSTALL=@INSTALL@
CFLAGS=@CFLAGS@
LDFLAGS=@LDFLAGS@
CLIBS=@LIBS@
INSTALLDIR=@prefix@

arctracker: arctracker.o initialise.o read_mod.o play_mod.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o arctracker arctracker.o initialise.o read_mod.o play_mod.o $(CLIBS)

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
