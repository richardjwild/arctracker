CC=@CC@
INSTALL=@INSTALL@
CFLAGS=@CFLAGS@ -Winline
LDFLAGS=@LDFLAGS@
CLIBS=@LIBS@
INSTALLDIR=@prefix@

arctracker: arctracker.o initialise.o read_mod.o play_mod.o write_audio.o resample.o mix.o gain.o oss.o alsa.o heap.o error.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o arctracker arctracker.o initialise.o read_mod.o play_mod.o write_audio.o resample.o mix.o gain.o oss.o alsa.o heap.o error.o $(CLIBS)

arctracker.o: arctracker.h audio_api.h arctracker.c
	$(CC) $(CFLAGS) -c arctracker.c

resample.o: resample.c heap.h
	$(CC) $(CFLAGS) -c resample.c

mix.o: mix.c
	$(CC) $(CFLAGS) -c mix.c

heap.o: heap.c
	$(CC) $(CFLAGS) -c heap.c

initialise.o: arctracker.h initialise.c error.h
	$(CC) $(CFLAGS) -c initialise.c

read_mod.o: arctracker.h read_mod.h read_mod.c
	$(CC) $(CFLAGS) -c read_mod.c

play_mod.o: arctracker.h log_lin_tab.h resample.h play_mod.h play_mod.c
	$(CC) $(CFLAGS) -c play_mod.c

write_audio.o: write_audio.c
	$(CC) $(CFLAGS) -c write_audio.c

gain.o: gain.c
	$(CC) $(CFLAGS) -c gain.c

oss.o: audio_api.h oss.c
	$(CC) $(CFLAGS) -c oss.c

alsa.o: config.h audio_api.h alsa.c
	$(CC) $(CFLAGS) -c alsa.c

error.o: error.h error.c
	$(CC) $(CFLAGS) -c error.c

clean:
	rm -f ./arctracker ./*.o 2>/dev/null

install: arctracker
	$(INSTALL) ./arctracker $(INSTALLDIR)/bin/arctracker

uninstall:
	rm -f $(INSTALLDIR)/bin/arctracker 2>/dev/null
