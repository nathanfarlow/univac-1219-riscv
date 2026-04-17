CC = gcc
CFLAGS = -O2 -fPIC -Wall -Werror
OBJS = mini_wrapper.o

all: libmini.so libmini.a

libmini.so: $(OBJS)
	$(CC) -shared -o $@ $^

libmini.a: $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) libmini.so libmini.a

.PHONY: all clean