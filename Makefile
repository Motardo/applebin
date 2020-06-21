P = applebin
OBJECTS = arguments.o singledouble.o
CFLAGS = -Wall -Wmissing-prototypes -g
LDLIBS=
CC=gcc

$(P): $(OBJECTS)
