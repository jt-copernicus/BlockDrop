# Makefile for BlockDrop
# A color-matching falling blocks puzzle game

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lncurses

# Installation directories
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man6

# Target binary
TARGET = blockdrop

# Source files
SRCS = blockdrop.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the game
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Install target
install: $(TARGET)
	@echo "Installing BlockDrop..."
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 $(TARGET).6 $(DESTDIR)$(MANDIR)/$(TARGET).6
	@echo "Installation complete!"
	@echo "Run '$(TARGET)' to start the game."

# Uninstall target
uninstall:
	@echo "Uninstalling BlockDrop..."
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(MANDIR)/$(TARGET).6
	@echo "Uninstallation complete!"

# Clean build files
clean:
	rm -f $(TARGET) $(OBJS)

# Clean and rebuild
rebuild: clean all

# Create distribution tarball
dist:
	tar -czf $(TARGET)-1.0.tar.gz $(SRCS) Makefile README.md $(TARGET).6

# Run the game (for development)
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS = -Wall -Wextra -g -O0 -std=c99 -DDEBUG
debug: $(TARGET)

# Static analysis with cppcheck (if available)
check:
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --suppress=missingIncludeSystem .; \
	else \
		echo "cppcheck not found, skipping static analysis"; \
	fi

# Help target
help:
	@echo "BlockDrop - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build the game (default)"
	@echo "  install      - Install the game and man page"
	@echo "  uninstall    - Remove the game and man page"
	@echo "  clean        - Remove build files"
	@echo "  rebuild      - Clean and rebuild"
	@echo "  run          - Build and run the game"
	@echo "  debug        - Build with debug symbols"
	@echo "  dist         - Create distribution tarball"
	@echo "  check        - Run static analysis (requires cppcheck)"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX       - Installation prefix (default: /usr/local)"
	@echo "  CC           - C compiler (default: gcc)"
	@echo "  CFLAGS       - Compiler flags"

.PHONY: all install uninstall clean rebuild run debug dist check help
