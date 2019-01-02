CC = gcc
CFLAGS = -Wall -Werror -g -std=c11
LDFLAGS = -lpthread -lbsd
INCLUDE =
SOURCES = main.c
TARGET = lab4.out


.PHONY: all clean

all: ${SOURCES}
	$(CC) ${CFLAGS} -o ${TARGET} ${SOURCES} ${LDFLAGS}

clean:
	@rm -rf ${TARGET}

