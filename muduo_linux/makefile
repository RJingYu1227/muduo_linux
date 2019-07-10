BASE_DIR = ./base
NET_DIR = ./net
HTTP_DIR = ./httpserver

INC_DIR = -I${BASE_DIR} \
	  -I${NET_DIR} \
	  -I${HTTP_DIR} 

SRC = $(wildcard ${BASE_DIR}/*.cpp) \
      $(wildcard ${NET_DIR}/*.cpp) \
      $(wildcard ${HTTP_DIR}/*.cpp) \
      ./main.cpp

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