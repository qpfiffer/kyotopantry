CPPFLAGS=-std=c++11 -Wall -Werror -g3 -O2
INCLUDES=-I./include -I./OlegDB/include
LIBS=-lzmq -lmsgpack -lm -lstdc++
NAME=kyotopantry
CC=clang
LIBOLEG=./OlegDB/build/lib/liboleg.so

all: $(NAME)

clean:
	rm *.o
	rm $(NAME)

%.o: ./src/%.cpp
	$(CC) $(CPPFLAGS) $(INCLUDES) -fpic -c $<

$(NAME): gatekeeper.o main.o
	$(CC) $(CPPFLAGS) $(INCLUDES) -o $(NAME) $^ $(LIBOLEG) $(LIBS)

