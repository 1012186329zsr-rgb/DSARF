# 头文件目录
DIR_INC = ./include
#源文件目录
DIR_SRC = ./src
#目标文件目录
DIR_OBJ = ./obj
#可执行文件目录
DIR_BIN = ./bin

SRC = $(wildcard ${DIR_SRC}/*.c)  
INC = $(wildcard ${DIR_INC}/*.h)
OBJ = $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${SRC})) 

TARGET = main

BIN_TARGET = ${DIR_BIN}/${TARGET}

CC = gcc
CFLAGS = -g -Wall -I${DIR_INC}

${BIN_TARGET}:${OBJ}
	$(CC) $(OBJ) -lm -o $@ -fopenmp

${DIR_OBJ}/%.o:${DIR_SRC}/%.c ${INC}
	$(CC) $(CFLAGS) -c  $< -o $@ -fopenmp

.PHONY:clean
clean:
	rm obj/*