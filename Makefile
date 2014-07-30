CPPFLAGS=-std=c++0x -pthread -Wall -Werror -Wshadow -g3
INCLUDES=-I./include -I./OlegDB/include
LIBS=-lzmq -lmsgpack -lm -lstdc++
NAME=kyotopantry
CC=g++
LIBOLEG=./OlegDB/build/lib/liboleg.so

all: $(NAME)

clean:
	rm *.o
	rm $(NAME)

%.o: ./src/%.c
	$(CC) $(CPPFLAGS) $(INCLUDES) -fpic -c $<

%.o: ./src/%.cpp
	$(CC) $(CPPFLAGS) $(INCLUDES) -fpic -c $<

$(NAME): BlueMidnightWish_ref.o vault.o jobtypes.o pikeman.o gatekeeper.o main.o
	$(CC) $(CPPFLAGS) $(INCLUDES) -o $(NAME) $^ $(LIBOLEG) $(LIBS)

