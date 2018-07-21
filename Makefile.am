CC = @CC@
INSTALL = @INSTALL@
CFLAGS = @CFLAGS@ -Winline
LDFLAGS = @LDFLAGS@
CLIBS = @LIBS@
INSTALLDIR = @prefix@

EXECUTABLE = arctracker

OBJS = \
arctracker.o \
configuration.o \
read_mod.o \
tracker_module.o \
desktop_tracker_module.o \
play_mod.o \
effects.o \
clock.o \
sequence.o \
console.o \
period.o \
write_audio.o \
resample.o \
mix.o \
gain.o \
oss.o \
alsa.o \
heap.o \
error.o

REBUILDABLES = $(EXECUTABLE) $(OBJS)

clean :
	rm -f $(EXECUTABLE) $(OBJS) 2>/dev/null
	@echo Clean done

all-local : $(EXECUTABLE)
	@echo All done

install : arctracker
	$(INSTALL) $(EXECUTABLE) $(INSTALLDIR)/bin/$(EXECUTABLE)
	@echo Install done

uninstall :
	rm -f $(INSTALLDIR)/bin/$(EXECUTABLE) 2>/dev/null
	@echo Uninstall done

$(EXECUTABLE) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(CLIBS)

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

arctracker.o : arctracker.c \
arctracker.h \
configuration.h \
read_mod.h \
play_mod.h \
oss.h \
alsa.h \
tracker_module.h \
desktop_tracker_module.h

configuration.o : configuration.c \
configuration.h \
error.h

resample.o : resample.c \
resample.h \
arctracker.h \
heap.h

mix.o : mix.c \
mix.h \
arctracker.h \
heap.h

heap.o : heap.c \
heap.h \
error.h

read_mod.o : read_mod.c \
read_mod.h \
arctracker.h \
config.h \
error.h \
configuration.h \
heap.h

tracker_module.o : tracker_module.c \
tracker_module.h \
arctracker.h \
error.h \
heap.h \
read_mod.h \
bits.h

desktop_tracker_module.o : desktop_tracker_module.c \
desktop_tracker_module.h \
heap.h \
read_mod.h \
arctracker.h \
bits.h

play_mod.o : play_mod.c \
play_mod.h \
arctracker.h \
config.h \
error.h \
write_audio.h \
sequence.h \
gain.h \
period.h \
configuration.h \
console.h \
clock.h \
audio_api.h \
heap.h \
effects.h

effects.o : effects.c \
effects.h \
arctracker.h \
clock.h \
sequence.h \
period.h \
gain.h \
bits.h \
play_mod.h

clock.o : clock.c \
clock.h

sequence.o : sequence.c \
sequence.h \
arctracker.h

console.o : console.c \
console.h \
arctracker.h \
bits.h \
sequence.h

period.o : period.c \
period.h \
error.h

write_audio.o : write_audio.c \
write_audio.h \
audio_api.h \
mix.h \
resample.h \
heap.h \
play_mod.h \
gain.h \
clock.h

gain.o : gain.c \
gain.h \
mu_law.h \
play_mod.h

oss.o : oss.c \
oss.h \
error.h

alsa.o : alsa.c \
alsa.h \
error.h \
config.h \
audio_api.h

error.o : error.c \
error.h
