CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lpulse

TARGET = pats
SOURCE = main.c

# Installation paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean install uninstall 
