CC=gcc
CFLAGS=-c -Os -std=c99 -static-libgcc

assemblies = enbrip
objects = enbrip.o

.PHONY : all clean clear

# Makefile "sugar" part
all : $(assemblies)

clean :
	@rm $(assemblies) $(objects)

clear : $(assemblies)
	@rm $(objects)

# Assembly part
enbrip : $(objects)
	$(CC) -s -o enbrip $(objects)

enbrip.o :
	$(CC) $(CFLAGS) enbrip.c
