P = applebin
OBJECTS = arguments.o singledouble.o
CFLAGS = -Wall -Wextra -Wmissing-prototypes -g
LDLIBS=
CC=gcc

$(P): $(OBJECTS)
