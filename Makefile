CC=gcc
CFLAGS=-c -Os -std=c99 -static-libgcc

BIN=bin
OBJ=obj
SRC=src

sources=$(wildcard $(SRC)/*.c)
objects=$(patsubst $(SRC)/%.c,$(OBJ)/%.obj,$(sources))

.PHONY: all clean clear

# Makefile "sugar" part
all: $(BIN)

clean:
	@if test -d $(BIN); then rm -rf $(BIN); fi
	@if test -d $(OBJ); then rm -rf $(OBJ); fi

clear: $(BIN)
	@if test -d $(OBJ); then rm -rf $(OBJ); fi

# enbrip Bin
$(BIN): $(OBJ) $(objects)
	@mkdir -p $(BIN)
	@if test -f $(BIN)/enbrip; then rm -rf $(BIN)/enbrip; fi
	$(CC) -s -static-libgcc -Wl,--gc-sections -o $(BIN)/enbrip $(objects) -lm

# enbrip Obj
$(OBJ):
	@mkdir -p $(OBJ)

$(OBJ)/%.obj: $(SRC)/%.c
	$(CC) $(CFLAGS) -o $@ $<

