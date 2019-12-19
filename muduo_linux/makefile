BASE_DIR = ./base
COROUTINE_DIR = ./coroutine
LOG_DIR = ./log
NET_DIR = ./net

INC_DIR = -I${BASE_DIR} \
          -I${COROUTINE_DIR} \
	      -I${LOG_DIR} \
	      -I${NET_DIR} \
	      -I"/usr/include/mysql"

SRC = $(wildcard ${BASE_DIR}/*.cpp) \
      $(wildcard ${COROUTINE_DIR}/*.cpp) \
	  $(wildcard ${LOG_DIR}/*.cpp) \
      $(wildcard ${NET_DIR}/*.cpp)

OBJ = $(patsubst %.cpp, %.o, ${SRC})

LIB = -lpthread -lmysqlclient

TARGET = webserver
CC = g++
FLAG = -Wall -std=c++11 ${INC_DIR} ${LIB}

${TARGET} : ${OBJ}
	${CC} -o $@ ${OBJ} ${FLAG}

${OBJ} : 
	${CC} -c -o $@ $(patsubst %.o, %.cpp, $@) ${FLAG}

clean:
	rm ${OBJ}
	rm ${TARGET}