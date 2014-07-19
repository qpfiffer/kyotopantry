CPPFLAGS=-std=c++0x -pthread -Wall -Werror -g3
INCLUDES=-I./include -I./OlegDB/include
LIBS=-lzmq -lmsgpack -lm -lstdc++
NAME=kyotopantry
CC=g++
LIBOLEG=./OlegDB/build/lib/liboleg.so

all: $(NAME)

clean:
	rm *.o
	rm $(NAME)

%.o: ./src/%.cpp
	$(CC) $(CPPFLAGS) $(INCLUDES) -fpic -c $<

$(NAME): pikeman.o gatekeeper.o main.o
	$(CC) $(CPPFLAGS) $(INCLUDES) -o $(NAME) $^ $(LIBOLEG) $(LIBS)

