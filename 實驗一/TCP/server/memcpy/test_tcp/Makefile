#define DEBUG macro in Makefile, use -D option
all: server
server: server.o
	gcc -o server server.c -lnsl -lpthread
clean:
	rm -f server server.o
