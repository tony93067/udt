#define DEBUG macro in Makefile, use -D option
all: sender
sender: func.o sender.o
	gcc -DDEBUG -o sender func.c sender.c -lnsl -lpthread
clean:
	rm -f sender func.o sender.o
