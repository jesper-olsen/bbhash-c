# Makefile for bbhash

CC      := clang
CFLAGS  := -std=c23 -Wall -Wextra -Wno-switch-enum -Wno-deprecated-non-prototype -O2 -DNDEBUG

ASTYLE  := astyle --suffix=none --align-pointer=name --pad-oper

# Define the common "library" source files
COMMON_SRC := bbhash.c mt64.c dedup.c hashing.c

# Define the headers to watch for changes
HEADERS := bitarray.h dedup.h mt64.h hashing.h bbhash.h

# Define the final executables
TARGETS := example example_strings

# The default 'make' command will build both targets
all: $(TARGETS)

# Rule to build the 'example' executable
example: example.c $(COMMON_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o example example.c $(COMMON_SRC)

# Rule to build the 'example_strings' executable
example_strings: example_strings.c $(COMMON_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o example_strings example_strings.c $(COMMON_SRC)

# Define separate 'run' commands for clarity
run-example: example
	./example 10000000

run-strings: example_strings
	./example_strings

fmt:
	@echo "Formatting source files..."
	$(ASTYLE) $(COMMON_SRC) example.c example_strings.c $(HEADERS) example_vocab.h

clean:
	rm -f $(TARGETS) *.o

.PHONY: all run-example run-strings clean fmt
