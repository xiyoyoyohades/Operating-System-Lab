CC = gcc
CFLAGS = -Wall -Werror -g -static -std=c11
LDFLAGS = -lpthread
INCLUDE =
#SOURCES = main_process.c main_thread.c
SOURCE1 = main_thread.c
SOURCE2 = main_process.c
TARGET1 = lab2_thread.out
TARGET2 = lab2_process.out

.PHONY: all clean

all: #${SOURCES}
	$(CC) ${CFLAGS} -o ${TARGET1} ${SOURCE1} ${LDFLAGS}
	$(CC) ${CFLAGS} -o ${TARGET2} ${SOURCE2} ${LDFLAGS}

clean:
	@rm -rf ${TARGET1}
	
run1:
	./${TARGET1}
	
run2:
	./${TARGET2}
