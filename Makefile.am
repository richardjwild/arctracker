CC=@CC@
INSTALL=@INSTALL@
CFLAGS=@CFLAGS@ -Winline
LDFLAGS=@LDFLAGS@
CLIBS=@LIBS@
INSTALLDIR=@prefix@

arctracker: arctracker.o initialise.o read_mod.o play_mod.o resample.o mix.o heap.o error.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o arctracker arctracker.o initialise.o read_mod.o play_mod.o resample.o mix.o heap.o error.o $(CLIBS)

arctracker.o: arctracker.h arctracker.c
	$(CC) $(CFLAGS) -c arctracker.c

resample.o: resample.c heap.h
	$(CC) $(CFLAGS) -c resample.c

mix.o: mix.c
	$(CC) $(CFLAGS) -c mix.c

heap.o: heap.c
	$(CC) $(CFLAGS) -c heap.c

initialise.o: arctracker.h initialise.c error.h
	$(CC) $(CFLAGS) -c initialise.c

read_mod.o: arctracker.h read_mod.c
	$(CC) $(CFLAGS) -c read_mod.c

play_mod.o: arctracker.h log_lin_tab.h resample.h play_mod.c
	$(CC) $(CFLAGS) -c play_mod.c

error.o: error.h error.c
	$(CC) $(CFLAGS) -c error.c

clean:
	rm -f ./arctracker ./*.o 2>/dev/null

install: arctracker
	$(INSTALL) ./arctracker $(INSTALLDIR)/bin/arctracker

uninstall:
	rm -f $(INSTALLDIR)/bin/arctracker 2>/dev/null
