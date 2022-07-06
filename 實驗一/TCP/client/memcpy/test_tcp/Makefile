#define DEBUG macro in Makefile, use -D option
all: client
client: client.o
	gcc -o client client.c -lnsl
clean:
	rm -f client client.o
