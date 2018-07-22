CC = @CC@
INSTALL = @INSTALL@
CFLAGS = @CFLAGS@ -Winline
LDFLAGS = @LDFLAGS@
CLIBS = @LIBS@
INSTALLDIR = @prefix@

TARGET = arctracker
SRCDIR = src
BINDIR = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h) $(wildcard $(SRCDIR)/*/*.h)

clean :
	rm -f $(BINDIR)/$(TARGET) 2>/dev/null
	@echo "Clean done"

all-local : $(BINDIR)/$(TARGET)
	@echo "All done"

install : all
	$(INSTALL) $(BINDIR)/$(TARGET) $(INSTALLDIR)/bin/$(TARGET)
	@echo "Install done"

uninstall :
	rm -f $(INSTALLDIR)/bin/$(TARGET) 2>/dev/null
	@echo "Uninstall done"

$(BINDIR)/$(TARGET) : $(SOURCES) $(INCLUDES)
	@if [ ! -d "$(BINDIR)" ]; then mkdir $(BINDIR); fi
	$(CC) -o $@ $(SOURCES) $(LDFLAGS) $(CLIBS)
