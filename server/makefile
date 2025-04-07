
TARGET:=CD_server

DIR_SRCS:=./source
DIR_INC:=./include
DIR_DEBUG:=./debug

SRCS:=$(wildcard ${DIR_SRCS}/*.c)
OBJS:=$(patsubst ${DIR_SRCS}/%.c,${DIR_DEBUG}/%.o,$(SRCS))
	
.PHONY:mk, clean	

ALL:mk $(TARGET)

mk:
	if [ ! -d "$(DIR_DEBUG)" ];then mkdir "$(DIR_DEBUG)";fi


$(TARGET):$(OBJS)
	gcc $^ -o $@ -lpthread -lmysqlclient -lcrypt -ll8w8jwt

$(DIR_DEBUG)/%.o:$(DIR_SRCS)/%.c
	gcc -c $^ -o $@ -I $(DIR_INC) 

clean:
	rm -r $(TARGET) $(DIR_DEBUG)