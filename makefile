BASE_DIR = ./pax/base
COROUTINE_DIR = ./pax/coroutine
LOG_DIR = ./pax/log
NET_DIR = ./pax/net

INC_DIR = -I"./"
	      -I"/usr/include/mysql"

SRC = $(wildcard ${BASE_DIR}/*.cpp) \
      $(wildcard ${COROUTINE_DIR}/*.cpp) \
	  $(wildcard ${LOG_DIR}/*.cpp) \
      $(wildcard ${NET_DIR}/*.cpp)

OBJ = $(patsubst %.cpp, %.o, ${SRC})

LIB = -lpthread -lmysqlclient

TARGET = libpax.a
CC = g++
FLAG = -Wall -std=c++11 -O0 ${INC_DIR} ${LIB}

${TARGET} : ${OBJ}
	ar cr $@ ${OBJ}

${OBJ} : 
	${CC} -c -o $@ $(patsubst %.o, %.cpp, $@) ${FLAG}

install:
	mkdir -p ./pax.build/include/ \
	&& mkdir ./pax.build/lib/ \
	&& cp ./${TARGET} ./pax.build/lib/ \
	&& cp -r ./pax ./pax.build/include/
	cd ./pax.build/include \
	&& rm ${OBJ} \
	&& rm ${SRC}

clean:
	rm ${OBJ}
	rm ${TARGET}