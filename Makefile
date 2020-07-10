
CC=clang
CPP=clang++
CCFLAGS=-g -O0
CPPFLAGS=-g -O0 -std=c++17
LDFLAGS=-lX11 -lXmu


@all: navio

@default: clean

clean: 
	rm -rf *.o *.obj navio

%.obj: %.cpp
	$(CPP) $(CPPFLAGS) -c $^ -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $^ -o $@

navio: navio.obj myX11.obj
	$(CPP) $(LDFLAGS) -o $@ $^

