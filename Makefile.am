CC = @CC@
INSTALL = @INSTALL@
CFLAGS = @CFLAGS@ -Winline
LDFLAGS = @LDFLAGS@
CLIBS = @LIBS@
INSTALLDIR = @prefix@

TARGET = arctracker
SRCDIR = src
OBJDIR = objs
BINDIR = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

clean :
	rm -f $(BINDIR)/$(TARGET) $(OBJECTS) 2>/dev/null
	@echo "Clean done"

all-local : $(BINDIR)/$(TARGET)
	@echo "All done"

install : all
	$(INSTALL) $(BINDIR)/$(TARGET) $(INSTALLDIR)/bin/$(TARGET)
	@echo "Install done"

uninstall :
	rm -f $(INSTALLDIR)/bin/$(TARGET) 2>/dev/null
	@echo "Uninstall done"

$(BINDIR)/$(TARGET) : $(OBJECTS)
	@if [ ! -d "$(BINDIR)" ]; then mkdir $(BINDIR); fi
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(CLIBS)

$(OBJECTS) : $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@if [ ! -d "$(OBJDIR)" ]; then mkdir $(OBJDIR); fi
	$(CC) $(CFLAGS) -c $< -o $@
