# Makefile for bbhash 

CC      := clang
#-DNDEBUG: no debug - disable assert
CFLAGS  := -std=c23 -Wall -Wextra -Wno-switch-enum -Wno-deprecated-non-prototype -O2 -DNDEBUG
#CFLAGS  := -std=c23 -Wall -Wextra -Wno-switch-enum -Wno-deprecated-non-prototype -O2 

ASTYLE  := astyle --suffix=none --align-pointer=name --pad-oper

SRC     := bbhash.c mt64.c bitarray.c dedup.c hashing.c example.c
HEADER  := mt64.h bitarray.h
TARGET  := example 

all: $(TARGET)

$(TARGET): $(SRC) $(HEADER)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET) 10000000

fmt:
	@echo "Formatting source files..."
	$(ASTYLE) $(SRC) $(HEADER)

clean:
	rm -f $(TARGET) *.o 

.PHONY: all run clean fmt

