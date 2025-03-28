
SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))

APP:cloudDisk_client

cloudDisk_client:$(OBJS)
	gcc $^ -o $@

%.o:%.c
	gcc -c $^ -o $@

clean:
	rm cloudDisk $(OBJS)