.PHONY: debug hello

all: debug hello

debug:
	gcc -g -o debug debug.c breakpoint.c -lpthread

hello:
	gcc -g -o hello hello.c -lpthread
