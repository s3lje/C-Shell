CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
TARGET = c-shell

# List your source files explicitly
# Adjust paths if your files are in subfolders
SRCS = c-shell.c include/builtins.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
