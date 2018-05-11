CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror -fPIC -fvisibility=hidden -g3
LIB = libmalloc.so
FILE = mymalloc.c
OBJS = mymalloc.o
VPATH = src2

all: $(OBJS)
	$(CC) -shared $(OBJS) -o $(LIB)

$(OBJS):
	$(CC) $(CFLAGS) $(VPATH)/$(FILE) -c -o $(OBJS)

clean:
	rm -f $(OBJS) $(LIB)
