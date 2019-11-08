BASE_DIR = ./base
COROUTINE_DIR = ./coroutine
NET_DIR = ./net
HTTP_DIR = ./http

INC_DIR = -I${BASE_DIR} \
      -I${COROUTINE_DIR} \
	  -I${NET_DIR} \
	  -I${HTTP_DIR} 

SRC = $(wildcard ${BASE_DIR}/*.cpp) \
      $(wildcard &{COROUTINE_DIR}/*.cpp) \
      $(wildcard ${NET_DIR}/*.cpp) \
      $(wildcard ${HTTP_DIR}/*.cpp)

OBJ = $(patsubst %.cpp, %.o, ${SRC})

LIB = -lpthread

TARGET = test0
CC = g++
FLAG = -Wall -std=c++11 ${INC_DIR} ${LIB}

${TARGET} : ${OBJ}
	${CC} -o $@ ${OBJ} ${FLAG}

${OBJ} : 
	${CC} -c -o $@ $(patsubst %.o, %.cpp, $@) ${FLAG}

clean:
	rm ${OBJ}
	rm ${TARGET}