TARGET = arctracker
SRCDIR = src
BINDIR = bin
EXECUTABLE = $(BINDIR)/$(TARGET)

CC = @CC@
INSTALL = @INSTALL@
CFLAGS = @CFLAGS@ -Winline -I $(SRCDIR)
LDFLAGS = @LDFLAGS@
CLIBS = @LIBS@
INSTALLDIR = @prefix@

SOURCES  := $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h) $(wildcard $(SRCDIR)/*/*.h)

clean :
	rm -f $(EXECUTABLE) 2>/dev/null
	@echo "Clean done"

all-local : $(EXECUTABLE)
	@echo "All done"

install : all
	$(INSTALL) $(EXECUTABLE) $(INSTALLDIR)/bin/$(TARGET)
	@echo "Install done"

uninstall :
	rm -f $(INSTALLDIR)/bin/$(TARGET) 2>/dev/null
	@echo "Uninstall done"

$(TARGET) : $(EXECUTABLE)

$(EXECUTABLE) : $(SOURCES) $(INCLUDES)
	@if [ ! -d "$(BINDIR)" ]; then mkdir $(BINDIR); fi
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS) $(CLIBS)
