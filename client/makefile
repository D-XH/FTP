
TARGET:=CD_client

DIR_SRCS:=./source
DIR_INC:=./include
DIR_DEBUG:=./debug

SRCS:=$(wildcard $(DIR_SRCS)/*.c)
OBJS:=$(patsubst $(DIR_SRCS)/%.c,$(DIR_DEBUG)/%.o,$(SRCS))
LIB:=-lpthread

.PHONY:APP, MKDIR, clean

APP:MKDIR $(TARGET)

$(TARGET):$(OBJS)
	gcc $^ -o $@ $(LIB)

$(DIR_DEBUG)/%.o:$(DIR_SRCS)/%.c
	gcc -c $^ -o $@ -I $(DIR_INC) 

MKDIR:
	mkdir -p $(DIR_DEBUG)

clean:
	rm -r $(TARGET) $(DIR_DEBUG)