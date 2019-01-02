CC = gcc
#CFLAGS = -Wall -Werror -g -static -std=c11 -DLOG_USE_COLOR
CFLAGS = -Wall -Werror  -std=c11
LDFLAGS = -lpthread
INCLUDE =
SOURCES = main.c
TARGET = lab3.out

.PHONY: all clean

all: ${SOURCES}
	$(CC) ${CFLAGS} -o ${TARGET} ${SOURCES} ${LDFLAGS}

clean:
	@rm -rf ${TARGET}
 
