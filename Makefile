CC = gcc
CFLAGS = -Wall -Iincludes -I../linked_list -I../hash_table

SRCS = $(wildcard src/*.c) interperter.c  ../data_structures/linked_list/linked_list.c ../data_structures/hash_table/hash_table.c
OBJS = $(SRCS:.c=.o)
DEPS = $(wildcard includes/*.h) ../data_structures/linked_list/linked_list.h ../data_structures/hash_table/hash_table.h

.PHONY: all clean

all: _run

_run: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
