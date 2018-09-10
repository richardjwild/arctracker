TARGET = arctracker
SRCDIR = src
BINDIR = bin
DISTDIR = dist
EXECUTABLE = $(BINDIR)/$(TARGET)
DISTFILES = aclocal.m4 CMakeLists.txt configure configure.ac COPYING install-sh Makefile.in Makefile.am README

CC = @CC@
INSTALL = @INSTALL@
CFLAGS = @CFLAGS@ -Winline -Wno-return-type -I $(SRCDIR)
LDFLAGS = @LDFLAGS@
CLIBS = @LIBS@
INSTALLDIR = @prefix@

SOURCES  := $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h) $(wildcard $(SRCDIR)/*/*.h)

clean :
	@rm -f $(EXECUTABLE) 2>/dev/null
	@rm -rf $(TARGET)-*.tar.gz
	@echo "Clean done"

all-local : $(EXECUTABLE)
	@echo "All done"

install : all
	$(INSTALL) $(EXECUTABLE) $(INSTALLDIR)/bin/$(TARGET)
	@echo "Install done"

uninstall :
	rm -f $(INSTALLDIR)/bin/$(TARGET) 2>/dev/null
	@echo "Uninstall done"

release :
	@if [ ! -d "$(DISTDIR)" ]; then mkdir $(DISTDIR); fi
	@cp -r $(SRCDIR) $(DISTDIR)/.
	@cp -t $(DISTDIR) $(DISTFILES)
	@mv $(DISTDIR) $(TARGET)-$(VERSION)
	@tar -czf $(TARGET)-$(VERSION).tar.gz $(TARGET)-$(VERSION)
	@rm -rf $(TARGET)-$(VERSION)
	@echo "Release done"

$(TARGET) : $(EXECUTABLE)

$(EXECUTABLE) : $(SOURCES) $(INCLUDES)
	@if [ ! -d "$(BINDIR)" ]; then mkdir $(BINDIR); fi
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS) $(CLIBS)
