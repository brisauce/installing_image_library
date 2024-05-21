
#	the C compiler i'm using, GCC
CC=gcc

#	-Wall specifies to the compiler to include all warning messages it could offer.
CFLAGS= -Wall -O0 -g

#	Include flags:
#	-IC:\msys64\mingw64\include\cairo is the directory where the files to be linked are.
#	-lcairo is the file i'm linking from there.
INCLUDE= -IC:\msys64\mingw64\include\libpng16

#	Linker flags:
LINKLIBS=-LC:\msys64\mingw64\lib -lpng

#	The name of the executable or binary I want to create.
BIN=main

#	the source files I will contribute to the executable.
SRC=main.c

#	the .o "object" files used to build the binary
#	I believe the ":.c=.o" implies the files mentioned in SRC are going to have
#	their .c extensions removed and replaced with .o extensions.
OBJS=$(SRC:.c=.o)

#	This is run when the "make" command is delivered. It's useful to remember that
#	when the "make" command is used, the first rule in the makefile is run, regardless
#	of its name.

#default:
#	@echo compiling test program
#	gcc -o main main.c -g

all: 
	$(CC) $(CFLAGS) $(INCLUDE) $(SRC) $(LINKLIBS) -o $(BIN)

clean:
	rm -f gray.png
