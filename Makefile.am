CC=@CC@
INSTALL=@INSTALL@
CFLAGS=@CFLAGS@ -Winline
LDFLAGS=@LDFLAGS@
CLIBS=@LIBS@
INSTALLDIR=@prefix@

arctracker: arctracker.o configuration.o read_mod.o tracker_module.o play_mod.o period.o write_audio.o resample.o mix.o gain.o oss.o alsa.o heap.o error.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o arctracker arctracker.o configuration.o read_mod.o tracker_module.o play_mod.o period.o write_audio.o resample.o mix.o gain.o oss.o alsa.o heap.o error.o $(CLIBS)

arctracker.o: arctracker.h audio_api.h arctracker.c
	$(CC) $(CFLAGS) -c arctracker.c

configuration.o: configuration.h error.h configuration.c
	$(CC) $(CFLAGS) -c configuration.c

resample.o: resample.c heap.h
	$(CC) $(CFLAGS) -c resample.c

mix.o: mix.c
	$(CC) $(CFLAGS) -c mix.c

heap.o: heap.c
	$(CC) $(CFLAGS) -c heap.c

read_mod.o: arctracker.h read_mod.h tracker_module.h read_mod.c
	$(CC) $(CFLAGS) -c read_mod.c

tracker_module.o: tracker_module.h error.h heap.h tracker_module.c
	$(CC) $(CFLAGS) -c tracker_module.c

play_mod.o: arctracker.h mu_law.h resample.h play_mod.h play_mod.c
	$(CC) $(CFLAGS) -c play_mod.c

period.o: period.h error.h period.c
	$(CC) $(CFLAGS) -c period.c

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
