## This is a simple Makefile with lost of comments 
## Check Unix Programming Tools handout for more info.

# Define what compiler to use and the flags.
CC=gcc
CXX=g++
CCFLAGS= -g -std=c99 -Wall -Werror -lrt -pthread

all: candykids

# Compile all .c files into .o files
# % matches all (like * in a command)
# $< is the source file (.c file)
%.o : %.c
	$(CC) -c $(CCFLAGS) $<

candykids: stats.o bbuff.o candykids.o
	$(CC) -o candykids stats.o bbuff.o candykids.o $(CCFLAGS)

clean:
	rm -f core *.o candykids