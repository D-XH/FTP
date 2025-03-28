
SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))

APP:cloudDisk

cloudDisk:$(OBJS)
	gcc $^ -o $@

%.o:%.c
	gcc -c $^ -o $@

clean:
	rm cloudDisk $(OBJS)